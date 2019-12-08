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
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _setCmdNoInOutUuid(Service* srv, Uuid *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
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

static Result _setCmdInU64NoOut(Service* srv, u64 inval, u32 cmd_id) {
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

Result setsysSetLanguageCode(u64 LanguageCode) {
    return _setCmdInU64NoOut(&g_setsysSrv, LanguageCode, 0);
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

Result setsysGetLockScreenFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 7);
}

Result setsysSetLockScreenFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 8);
}

Result setsysGetAccountSettings(SetSysAccountSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 17, *out);
}

Result setsysSetAccountSettings(SetSysAccountSettings settings) {
    return serviceDispatchIn(&g_setsysSrv, 18, settings);
}

Result setsysGetEulaVersions(s32 *total_out, SetSysEulaVersion *versions, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 21, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { versions, count*sizeof(SetSysEulaVersion) } },
    );
}

Result setsysSetEulaVersions(const SetSysEulaVersion *versions, s32 count) {
    return serviceDispatch(&g_setsysSrv, 22,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { versions, count*sizeof(SetSysEulaVersion) } },
    );
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

Result setsysGetConsoleInformationUploadFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 25);
}

Result setsysSetConsoleInformationUploadFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 26);
}

Result setsysGetAutomaticApplicationDownloadFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 27);
}

Result setsysSetAutomaticApplicationDownloadFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 28);
}

Result setsysGetNotificationSettings(SetSysNotificationSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 29, *out);
}

Result setsysSetNotificationSettings(const SetSysNotificationSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 30, *settings);
}

Result setsysGetAccountNotificationSettings(s32 *total_out, SetSysAccountNotificationSettings *settings, s32 count) {
    return serviceDispatchOut(&g_setsysSrv, 31, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { settings, count*sizeof(SetSysAccountNotificationSettings) } },
    );
}

Result setsysSetAccountNotificationSettings(const SetSysAccountNotificationSettings *settings, s32 count) {
    return serviceDispatch(&g_setsysSrv, 32,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { settings, count*sizeof(SetSysAccountNotificationSettings) } },
    );
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

Result setsysGetTvSettings(SetSysTvSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 39, *out);
}

Result setsysSetTvSettings(const SetSysTvSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 40, *settings);
}

Result setsysGetQuestFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 47);
}

Result setsysSetQuestFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 48);
}

Result setsysGetDataDeletionSettings(SetSysDataDeletionSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 49, *out);
}

Result setsysSetDataDeletionSettings(const SetSysDataDeletionSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 50, *settings);
}

Result setsysGetWirelessCertificationFileSize(u64 *out_size) {
    return _setCmdNoInOut64(&g_setsysSrv, out_size, 55);
}

Result setsysGetWirelessCertificationFile(void* buffer, size_t size, u64 *out_size) {
    return serviceDispatchOut(&g_setsysSrv, 56, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );
}

Result setsysSetRegionCode(SetRegion region) {
    return _setCmdInU32NoOut(&g_setsysSrv, region, 57);
}

Result setsysIsUserSystemClockAutomaticCorrectionEnabled(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 60);
}

Result setsysSetUserSystemClockAutomaticCorrectionEnabled(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 61);
}

Result setsysGetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage *out) {
    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 63);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage storage) {
    return _setCmdInU32NoOut(&g_setsysSrv, storage, 64);
}

Result setsysGetUsb30EnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 65);
}

Result setsysSetUsb30EnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 66);
}

Result setsysGetBatteryLot(SetBatteryLot *out) {
    return serviceDispatchOut(&g_setsysSrv, 67, *out);
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

Result setsysGetNfcEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 69);
}

Result setsysSetNfcEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 70);
}

Result setsysGetSleepSettings(SetSysSleepSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 71, *out);
}

Result setsysSetSleepSettings(const SetSysSleepSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 72, *settings);
}

Result setsysGetWirelessLanEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 73);
}

Result setsysSetWirelessLanEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 74);
}

Result setsysGetInitialLaunchSettings(SetSysInitialLaunchSettings *out) {
    return serviceDispatchOut(&g_setsysSrv, 75, *out);
}

Result setsysSetInitialLaunchSettings(const SetSysInitialLaunchSettings *settings) {
    return serviceDispatchIn(&g_setsysSrv, 76, *settings);
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

Result setsysGetProductModel(s32 *out) {
    return _setCmdNoInOutU32(&g_setsysSrv, (u32*)out, 79);
}

Result setsysGetBluetoothEnableFlag(bool *out) {
    return _setCmdNoInOutBool(&g_setsysSrv, out, 88);
}

Result setsysSetBluetoothEnableFlag(bool flag) {
    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 89);
}

Result setsysGetMiiAuthorId(Uuid *out) {
    return _setCmdNoInOutUuid(&g_setsysSrv, out, 90);
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

Result setsysGetAutoUpdateEnableFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 95);
}

Result setsysSetAutoUpdateEnableFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 96);
}

Result setsysGetBatteryPercentageFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 99);
}

Result setsysSetBatteryPercentageFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 100);
}

Result setsysGetExternalRtcResetFlag(bool *out) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 101);
}

Result setsysSetExternalRtcResetFlag(bool flag) {
    if (hosversionBefore(2,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 102);
}

Result setsysGetUsbFullKeyEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 103);
}

Result setsysSetUsbFullKeyEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 104);
}

Result setsysGetBluetoothAfhEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 111);
}

Result setsysSetBluetoothAfhEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 112);
}

Result setsysGetBluetoothBoostEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 113);
}

Result setsysSetBluetoothBoostEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 114);
}

Result setsysGetInRepairProcessEnableFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 115);
}

Result setsysSetInRepairProcessEnableFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 116);
}

Result setsysGetHeadphoneVolumeUpdateFlag(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 117);
}

Result setsysSetHeadphoneVolumeUpdateFlag(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 118);
}

Result setsysGetErrorReportSharePermission(SetSysErrorReportSharePermission *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 124);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetErrorReportSharePermission(SetSysErrorReportSharePermission permission) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, permission, 125);
}

Result setsysGetAppletLaunchFlags(u32 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 126);
}

Result setsysSetAppletLaunchFlags(u32 flags) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, flags, 127);
}

Result setsysGetKeyboardLayout(SetKeyboardLayout *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 136);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetKeyboardLayout(SetKeyboardLayout layout) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, layout, 137);
}

Result setsysGetRequiresRunRepairTimeReviser(bool *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 141);
}

Result setsysGetRebootlessSystemUpdateVersion(SetSysRebootlessSystemUpdateVersion *out) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 149, *out);
}

Result setsysSetRequiresRunRepairTimeReviser(bool flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 142);
}

Result setsysGetPctlReadyFlag(bool *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 156);
}

Result setsysSetPctlReadyFlag(bool flag) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInBoolNoOut(&g_setsysSrv, flag, 157);
}

Result setsysGetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod *out) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 170);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod method) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, method, 171);
}

Result setsysGetHomeMenuScheme(SetSysHomeMenuScheme *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_setsysSrv, 174, *out);
}

Result setsysGetPlatformRegion(SetSysPlatformRegion *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 183);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetPlatformRegion(SetSysPlatformRegion region) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, region, 184);
}

Result setsysGetHomeMenuSchemeModel(u32 *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutU32(&g_setsysSrv, out, 185);
}

Result setsysGetMemoryUsageRateFlag(bool *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdNoInOutBool(&g_setsysSrv, out, 186);
}

Result setsysGetTouchScreenMode(SetSysTouchScreenMode *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = _setCmdNoInOutU32(&g_setsysSrv, &tmp, 187);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result setsysSetTouchScreenMode(SetSysTouchScreenMode mode) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _setCmdInU32NoOut(&g_setsysSrv, mode, 188);
}
