#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/set.h"
#include "services/applet.h"

static Service g_setSrv;
static Service g_setsysSrv;

static bool g_setLanguageCodesInitialized;
static u64 g_setLanguageCodes[0x40];
static s32 g_setLanguageCodesTotal;

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode);

NX_GENERATE_SERVICE_GUARD(set);

Result _setInitialize(void) {
    g_setLanguageCodesInitialized = 0;

    return smGetService(&g_setSrv, "set");
}

void _setCleanup(void) {
    serviceClose(&g_setSrv);
}

Service* setGetServiceSession(void) {
    return &g_setSrv;
}

NX_GENERATE_SERVICE_GUARD(setsys);

Result _setsysInitialize(void) {
    return smGetService(&g_setsysSrv, "set:sys");
}

void _setsysCleanup(void) {
    serviceClose(&g_setsysSrv);
}

Service* setsysGetServiceSession(void) {
    return &g_setsysSrv;
}

static Result _setCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _setCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _setCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _setCmdNoInOut64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _setCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp!=0;
    return rc;
}

static Result _setCmdInU8NoOut(Service* srv, u8 inval, u64 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _setCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _setCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _setCmdInU32NoOut(Service* srv, u32 inval, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result setInitializeLanguageCodesCache(void) {
    if (g_setLanguageCodesInitialized) return 0;
    Result rc = 0;

    rc = setGetAvailableLanguageCodes(&g_setLanguageCodesTotal, g_setLanguageCodes, sizeof(g_setLanguageCodes)/sizeof(u64));
    if (R_FAILED(rc)) return rc;

    if (g_setLanguageCodesTotal < 0) g_setLanguageCodesTotal = 0;

    g_setLanguageCodesInitialized = 1;

    return rc;
}

Result setMakeLanguage(u64 LanguageCode, SetLanguage *Language) {
    Result rc = setInitializeLanguageCodesCache();
    if (R_FAILED(rc)) return rc;

    s32 i;
    rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    for (i=0; i<g_setLanguageCodesTotal; i++) {
        if (g_setLanguageCodes[i] == LanguageCode) {
            *Language = i;
            return 0;
        }
    }

    return rc;
}

Result setMakeLanguageCode(SetLanguage Language, u64 *LanguageCode) {
    Result rc = setInitializeLanguageCodesCache();
    if (R_FAILED(rc)) return rc;

    if (Language < 0)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    if (Language >= g_setLanguageCodesTotal) {
        if (hosversionBefore(4,0,0)) return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        return _setMakeLanguageCode(Language, LanguageCode);
    }

    *LanguageCode = g_setLanguageCodes[Language];

    return rc;
}

Result setGetSystemLanguage(u64 *LanguageCode) {
    //This is disabled because the returned LanguageCode can differ from the system language, for example ja instead of {English}.
    /*Result rc = appletGetDesiredLanguage(LanguageCode);
    if (R_SUCCEEDED(rc)) return rc;*/

    return setGetLanguageCode(LanguageCode);
}

Result setGetLanguageCode(u64 *LanguageCode) {
    return _setCmdNoInOut64(&g_setSrv, LanguageCode, 0);
}

Result setGetAvailableLanguageCodes(s32 *total_entries, u64 *LanguageCodes, size_t max_entries) {
    Result rc=0;
    bool new_cmd = hosversionAtLeast(4,0,0);

    if (!new_cmd) {//On system-version <4.0.0 the sysmodule will close the session if max_entries is too large.
        s32 tmptotal = 0;

        rc = setGetAvailableLanguageCodeCount(&tmptotal);
        if (R_FAILED(rc)) return rc;

        if (max_entries > (size_t)tmptotal) max_entries = (size_t)tmptotal;
    }

    return serviceDispatchOut(&g_setSrv, new_cmd ? 5 : 1, *total_entries,
        .buffer_attrs = { (new_cmd ? SfBufferAttr_HipcMapAlias : SfBufferAttr_HipcPointer) | SfBufferAttr_Out },
        .buffers = { { LanguageCodes, max_entries*sizeof(u64) } },
    );
}

static Result _setMakeLanguageCode(s32 Language, u64 *LanguageCode) {
    return serviceDispatchInOut(&g_setSrv, 2, Language, *LanguageCode);
}

Result setGetAvailableLanguageCodeCount(s32 *total) {
    Result rc = _setCmdNoInOutU32(&g_setSrv, (u32*)total, hosversionAtLeast(4,0,0) ? 6 : 3);
    if (R_SUCCEEDED(rc) && total && *total < 0) *total = 0;
    return rc;
}

Result setGetRegionCode(SetRegion *out) {
    s32 code=0;
    Result rc = _setCmdNoInOutU32(&g_setSrv, (u32*)&code, 4);
    if (R_SUCCEEDED(rc) && out) *out = code;
    return rc;
}

Result setsysGetColorSetId(ColorSetId *out) {
    u32 color_set=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &color_set, 23);
    if (R_SUCCEEDED(rc) && out) *out = color_set;
    return rc;
}

Result setsysSetColorSetId(ColorSetId id) {
    return _setCmdInU32NoOut(&g_setsysSrv, id, 24);
}

Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];
    
    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    return serviceDispatchOut(&g_setsysSrv, 37, *size_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { send_name, SET_MAX_NAME_SIZE },
            { send_item_key, SET_MAX_NAME_SIZE },
        },
    );
}

Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];
    
    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    return serviceDispatchOut(&g_setsysSrv, 38, *size_out,
        .buffer_attrs = {
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcPointer | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
        },
        .buffers = {
            { send_name, SET_MAX_NAME_SIZE },
            { send_item_key, SET_MAX_NAME_SIZE },
            { value_out, value_out_size },
        },
    );
}

Result setsysGetSerialNumber(char *serial) {
    char out[0x18]={0};

    Result rc = serviceDispatchOut(&g_setsysSrv, 68, out);
    if (R_SUCCEEDED(rc) && serial) {
        memcpy(serial, out, 0x18);
        serial[0x18]=0;
    }
    return rc;
}

Result setsysGetFlag(SetSysFlag flag, bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, flag);
}

Result setsysSetFlag(SetSysFlag flag, bool enable) {
    return _setCmdInBoolNoOut(&g_setsysSrv, enable, flag + 1);
}

static Result _setsysGetFirmwareVersionImpl(SetSysFirmwareVersion *out, u32 cmd_id) {
    return serviceDispatch(&g_setsysSrv, cmd_id,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result setsysGetFirmwareVersion(SetSysFirmwareVersion *out) {
    /* GetFirmwareVersion2 does exactly what GetFirmwareVersion does, except it doesn't zero the revision field. */
    if (hosversionAtLeast(3,0,0)) {
        return _setsysGetFirmwareVersionImpl(out, 4);
    } else {
        return _setsysGetFirmwareVersionImpl(out, 3);
    }
}

Result setsysBindFatalDirtyFlagEvent(Event *out_event) {
    return _setCmdGetEvent(&g_setsysSrv, out_event, false, 93);
}

Result setsysGetFatalDirtyFlags(u64 *flags_0, u64 *flags_1) {
    struct {
        u64 flags_0;
        u64 flags_1;
    } out;

    Result rc = serviceDispatchOut(&g_setsysSrv, 94, out);
    if (R_SUCCEEDED(rc) && flags_0) *flags_0 = out.flags_0;
    if (R_SUCCEEDED(rc) && flags_1) *flags_1 = out.flags_1;
    return rc;
}

Result setsysGetDeviceNickname(char* nickname) {
    return serviceDispatch(&g_setsysSrv, 77,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { nickname, SET_MAX_NICKNAME_SIZE } },
    );
}

Result setsysSetDeviceNickname(const char* nickname) {
    char send_nickname[SET_MAX_NICKNAME_SIZE] = {0};
    strncpy(send_nickname, nickname, SET_MAX_NICKNAME_SIZE-1);

    return serviceDispatch(&g_setsysSrv, 78,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { send_nickname, SET_MAX_NICKNAME_SIZE } },
    );
}
