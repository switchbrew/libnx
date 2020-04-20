#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include <string.h>
#include "services/applet.h"
#include "services/hidsys.h"
#include "runtime/hosversion.h"

static Service g_hidsysSrv;

static u64 g_hidsysAppletResourceUserId = 0;

NX_GENERATE_SERVICE_GUARD(hidsys);

Result _hidsysInitialize(void) {
    Result rc = smGetService(&g_hidsysSrv, "hid:sys");
    if (R_FAILED(rc))
        return rc;

    g_hidsysAppletResourceUserId = appletGetAppletResourceUserId();
    return 0;
}

void _hidsysCleanup(void) {
    serviceClose(&g_hidsysSrv);
}

Service* hidsysGetServiceSession(void) {
    return &g_hidsysSrv;
}

static Result _hidsysCmdWithResIdAndPid(u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, g_hidsysAppletResourceUserId,
        .in_send_pid = true,
    );
}

static Result _hidsysCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_hidsysSrv, cmd_id);
}

static Result _hidsysCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval);
}

static Result _hidsysCmdInU64NoOut(u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval);
}

static Result _hidsysCmdInU64InBoolNoOut(u64 inval, bool flag, u32 cmd_id) {
    const struct {
        u8 flag;
        u8 pad[7];
        u64 inval;
    } in = { flag!=0, {0}, inval };

    return serviceDispatchIn(&g_hidsysSrv, cmd_id, in);
}

static Result _hidsysCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_hidsysSrv, cmd_id, *out);
}

static Result _hidsysCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _hidsysCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInU32OutBool(u32 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, cmd_id, inval, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInU64OutBool(u64 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, cmd_id, inval, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdGetHandle(Handle* handle_out, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, g_hidsysAppletResourceUserId,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _hidsysCmdGetEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _hidsysCmdGetHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _hidsysCmdInU64InBufNoOut(u64 inval, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInBufOutBool(const void* buf, size_t size, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_hidsysSrv, cmd_id, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, size } },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInU32InBufNoOut(u32 inval, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInU32OutBuf(u32 inval, void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInU64OutBuf(u64 inval, void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

Result hidsysAcquireHomeButtonEventHandle(Event* out_event) {
    return _hidsysCmdGetEvent(out_event, false, 101);
}

//These functions don't seem to work in the overlaydisp applet context
Result hidsysAcquireSleepButtonEventHandle(Event* out_event) {
    return _hidsysCmdGetEvent(out_event, false, 121);
}

Result hidsysAcquireCaptureButtonEventHandle(Event* out_event) {
    return _hidsysCmdGetEvent(out_event, false, 141);
}

Result hidsysActivateHomeButton(void) {
    return _hidsysCmdWithResIdAndPid(111);
}

Result hidsysActivateSleepButton(void) {
    return _hidsysCmdWithResIdAndPid(131);
}

Result hidsysActivateCaptureButton(void) {
    return _hidsysCmdWithResIdAndPid(151);
}

static Result _hidsysGetMaskedSupportedNpadStyleSet(u64 AppletResourceUserId, HidControllerType *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 310, AppletResourceUserId, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result hidsysGetSupportedNpadStyleSetOfCallerApplet(HidControllerType *out) {
    u64 AppletResourceUserId=0;
    Result rc=0;

    rc = appletGetAppletResourceUserIdOfCallerApplet(&AppletResourceUserId);
    if (R_FAILED(rc) && rc != MAKERESULT(128, 82)) return rc;

    return _hidsysGetMaskedSupportedNpadStyleSet(AppletResourceUserId, out);
}

Result hidsysGetUniquePadsFromNpad(HidControllerID id, u64 *UniquePadIds, s32 count, s32 *total_entries) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=hidControllerIDToOfficial(id);
    s64 out=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 321, tmp, out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { UniquePadIds, count*sizeof(u64) } },
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = out;
    return rc;
}

Result hidsysEnableAppletToGetInput(bool enable) {
    const struct {
        u8 permitInput;
        u64 appletResourceUserId;
    } in = { enable!=0, g_hidsysAppletResourceUserId };

    return serviceDispatchIn(&g_hidsysSrv, 503, in);
}

Result hidsysGetUniquePadIds(u64 *UniquePadIds, s32 count, s32 *total_entries) {
    s64 out=0;
    Result rc = serviceDispatchOut(&g_hidsysSrv, 703, out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { UniquePadIds, count*sizeof(u64) } },
    );
    if (R_SUCCEEDED(rc) && total_entries) *total_entries = out;
    return rc;
}

Result hidsysGetUniquePadSerialNumber(u64 UniquePadId, char *serial) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    char out[0x10]={0};
    if (serial) memset(serial, 0, 0x11);
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 809, UniquePadId, out);
    if (R_SUCCEEDED(rc) && serial) memcpy(serial, out, 0x10);
    return rc;
}

Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, u64 UniquePadId) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidsysNotificationLedPattern pattern;
        u64 UniquePadId;
    } in = { *pattern, UniquePadId };

    return serviceDispatchIn(&g_hidsysSrv, 830, in);
}

Result hidsysSetNotificationLedPatternWithTimeout(const HidsysNotificationLedPattern *pattern, u64 UniquePadId, u64 timeout) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidsysNotificationLedPattern pattern;
        u64 UniquePadId;
        u64 timeout;
    } in = { *pattern, UniquePadId, timeout };

    return serviceDispatchIn(&g_hidsysSrv, 831, in);
}

Result hidsysIsButtonConfigSupported(u64 unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id, out, 1200);
}

Result hidsysDeleteButtonConfig(u64 unique_pad_id) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64NoOut(unique_pad_id, 1201);
}

Result hidsysSetButtonConfigEnabled(u64 unique_pad_id, bool flag) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBoolNoOut(unique_pad_id, flag, 1202);
}

Result hidsysIsButtonConfigEnabled(u64 unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id, out, 1203);
}

Result hidsysSetButtonConfigEmbedded(u64 unique_pad_id, const HidsysButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1204);
}

Result hidsysSetButtonConfigFull(u64 unique_pad_id, const HidsysButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1205);
}

Result hidsysSetButtonConfigLeft(u64 unique_pad_id, const HidsysButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1206);
}

Result hidsysSetButtonConfigRight(u64 unique_pad_id, const HidsysButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1207);
}

Result hidsysGetButtonConfigEmbedded(u64 unique_pad_id, HidsysButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1208);
}

Result hidsysGetButtonConfigFull(u64 unique_pad_id, HidsysButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1209);
}

Result hidsysGetButtonConfigLeft(u64 unique_pad_id, HidsysButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1210);
}

Result hidsysGetButtonConfigRight(u64 unique_pad_id, HidsysButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1211);
}

Result hidsysIsCustomButtonConfigSupported(u64 unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id, out, 1250);
}

Result hidsysIsDefaultButtonConfigEmbedded(const HidcfgButtonConfigEmbedded *config, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBufOutBool(config, sizeof(*config), out, 1251);
}

Result hidsysIsDefaultButtonConfigFull(const HidcfgButtonConfigFull *config, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBufOutBool(config, sizeof(*config), out, 1252);
}

Result hidsysIsDefaultButtonConfigLeft(const HidcfgButtonConfigLeft *config, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBufOutBool(config, sizeof(*config), out, 1253);
}

Result hidsysIsDefaultButtonConfigRight(const HidcfgButtonConfigRight *config, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBufOutBool(config, sizeof(*config), out, 1254);
}

Result hidsysIsButtonConfigStorageEmbeddedEmpty(s32 index, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBool((u32)index, out, 1255);
}

Result hidsysIsButtonConfigStorageFullEmpty(s32 index, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBool((u32)index, out, 1256);
}

Result hidsysIsButtonConfigStorageLeftEmpty(s32 index, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBool((u32)index, out, 1257);
}

Result hidsysIsButtonConfigStorageRightEmpty(s32 index, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBool((u32)index, out, 1258);
}

Result hidsysGetButtonConfigStorageEmbedded(s32 index, HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBuf((u32)index, config, sizeof(*config), 1259);
}

Result hidsysGetButtonConfigStorageFull(s32 index, HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBuf((u32)index, config, sizeof(*config), 1260);
}

Result hidsysGetButtonConfigStorageLeft(s32 index, HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBuf((u32)index, config, sizeof(*config), 1261);
}

Result hidsysGetButtonConfigStorageRight(s32 index, HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBuf((u32)index, config, sizeof(*config), 1262);
}

Result hidsysSetButtonConfigStorageEmbedded(s32 index, const HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufNoOut((u32)index, config, sizeof(*config), 1263);
}

Result hidsysSetButtonConfigStorageFull(s32 index, const HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufNoOut((u32)index, config, sizeof(*config), 1264);
}

Result hidsysSetButtonConfigStorageLeft(s32 index, const HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufNoOut((u32)index, config, sizeof(*config), 1265);
}

Result hidsysSetButtonConfigStorageRight(s32 index, const HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufNoOut((u32)index, config, sizeof(*config), 1266);
}

Result hidsysDeleteButtonConfigStorageEmbedded(s32 index) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32NoOut((u32)index, 1267);
}

Result hidsysDeleteButtonConfigStorageFull(s32 index) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32NoOut((u32)index, 1268);
}

Result hidsysDeleteButtonConfigStorageLeft(s32 index) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32NoOut((u32)index, 1269);
}

Result hidsysDeleteButtonConfigStorageRight(s32 index) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32NoOut((u32)index, 1270);
}

Result hidsysIsUsingCustomButtonConfig(u64 unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id, out, 1271);
}

Result hidsysIsAnyCustomButtonConfigEnabled(u64 unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoInOutBool(out, 1272);
}

Result hidsysSetAllCustomButtonConfigEnabled(u64 AppletResourceUserId, bool flag) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBoolNoOut(AppletResourceUserId, flag, 1273);
}

Result hidsysSetDefaultButtonConfig(u64 unique_pad_id) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64NoOut(unique_pad_id, 1274);
}

Result hidsysSetAllDefaultButtonConfig(void) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoIO(1275);
}

Result hidsysSetHidButtonConfigEmbedded(u64 unique_pad_id, const HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1276);
}

Result hidsysSetHidButtonConfigFull(u64 unique_pad_id, const HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1277);
}

Result hidsysSetHidButtonConfigLeft(u64 unique_pad_id, const HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1278);
}

Result hidsysSetHidButtonConfigRight(u64 unique_pad_id, const HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufNoOut(unique_pad_id, config, sizeof(*config), 1279);
}

Result hidsysGetHidButtonConfigEmbedded(u64 unique_pad_id, HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1280);
}

Result hidsysGetHidButtonConfigFull(u64 unique_pad_id, HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1281);
}

Result hidsysGetHidButtonConfigLeft(u64 unique_pad_id, HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1282);
}

Result hidsysGetHidButtonConfigRight(u64 unique_pad_id, HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBuf(unique_pad_id, config, sizeof(*config), 1283);
}

