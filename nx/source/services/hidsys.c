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

static Result _hidsysCmdInU8NoOut(u8 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval);
}

static Result _hidsysCmdInBoolNoOut(bool flag, u32 cmd_id) {
    return _hidsysCmdInU8NoOut(flag!=0, cmd_id);
}

static Result _hidsysCmdInU32NoOut(u32 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval);
}

static Result _hidsysCmdInU64NoOut(u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval);
}

static Result _hidsysCmdInAddrNoOut(BtdrvAddress addr, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, addr);
}

static Result _hidsysCmdInU64InBoolNoOut(u64 inval, bool flag, u32 cmd_id) {
    const struct {
        u8 flag;
        u8 pad[7];
        u64 inval;
    } in = { flag!=0, {0}, inval };

    return serviceDispatchIn(&g_hidsysSrv, cmd_id, in);
}

static Result _hidsysCmdInAddrInBoolNoOut(BtdrvAddress addr, bool flag, u32 cmd_id) {
    const struct {
        u8 flag;
        BtdrvAddress addr;
    } in = { flag!=0, addr };

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

static Result _hidsysCmdInU32OutU8(u32 inval, u8 *out, u32 cmd_id) {
    return serviceDispatchInOut(&g_hidsysSrv, cmd_id, inval, *out);
}

static Result _hidsysCmdInU32OutBool(u32 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _hidsysCmdInU32OutU8(inval, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInU32OutU8U8(u32 inval, u8 *out0, u8 *out1, u32 cmd_id) {
    struct {
        u8 out0;
        u8 out1;
        u8 pad[2];
    } out;

    Result rc = serviceDispatchInOut(&g_hidsysSrv, cmd_id, inval, out);
    if (R_SUCCEEDED(rc)) {
        if (out0) *out0 = out.out0;
        if (out0) *out1 = out.out1;
    }
    return rc;
}

static Result _hidsysCmdInU64OutBool(u64 inval, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, cmd_id, inval, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInAddrOutBool(BtdrvAddress addr, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, cmd_id, addr, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _hidsysCmdInPidAruidOutHandle(Handle* handle_out, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, g_hidsysAppletResourceUserId,
        .in_send_pid = true,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _hidsysCmdInPidAruidOutEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _hidsysCmdInPidAruidOutHandle(&tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _hidsysCmdInU64InBufFixedNoOut(u64 inval, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInAddrInBufFixedNoOut(BtdrvAddress addr, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, addr,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
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

static Result _hidsysCmdInU32InBufFixedNoOut(u32 inval, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInU32InBufFixedInPtrFixedNoOut(u32 inval, const void* buf0, size_t size0, const void* buf1, size_t size1, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = {
            SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
            SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In,
        },
        .buffers = {
            { buf0, size0 },
            { buf1, size1 },
        },
    );
}

static Result _hidsysCmdInU32OutBufFixed(u32 inval, void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInU32OutBufFixedOutPtrFixed(u32 inval, void* buf0, size_t size0, void* buf1, size_t size1, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = {
            SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out,
        },
        .buffers = {
            { buf0, size0 },
            { buf1, size1 },
        },
    );
}

static Result _hidsysCmdInU64OutBufFixed(u64 inval, void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, inval,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

static Result _hidsysCmdInAddrOutBufFixedNoOut(BtdrvAddress addr, const void* buf, size_t size, u32 cmd_id) {
    return serviceDispatchIn(&g_hidsysSrv, cmd_id, addr,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, size } },
    );
}

Result hidsysSendKeyboardLockKeyEvent(u32 events) {
    return _hidsysCmdInU32NoOut(events, 31);
}

Result hidsysAcquireHomeButtonEventHandle(Event* out_event, bool autoclear) {
    return _hidsysCmdInPidAruidOutEvent(out_event, autoclear, 101);
}

Result hidsysActivateHomeButton(void) {
    return _hidsysCmdWithResIdAndPid(111);
}

Result hidsysAcquireSleepButtonEventHandle(Event* out_event, bool autoclear) {
    return _hidsysCmdInPidAruidOutEvent(out_event, autoclear, 121);
}

Result hidsysActivateSleepButton(void) {
    return _hidsysCmdWithResIdAndPid(131);
}

Result hidsysAcquireCaptureButtonEventHandle(Event* out_event, bool autoclear) {
    return _hidsysCmdInPidAruidOutEvent(out_event, autoclear, 141);
}

Result hidsysActivateCaptureButton(void) {
    return _hidsysCmdWithResIdAndPid(151);
}

static Result _hidsysGetMaskedSupportedNpadStyleSet(u64 AppletResourceUserId, u32 *out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 310, AppletResourceUserId, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result hidsysGetSupportedNpadStyleSetOfCallerApplet(u32 *out) {
    u64 AppletResourceUserId=0;
    Result rc=0;

    rc = appletGetAppletResourceUserIdOfCallerApplet(&AppletResourceUserId);
    if (R_FAILED(rc) && rc != MAKERESULT(128, 82)) return rc;

    return _hidsysGetMaskedSupportedNpadStyleSet(AppletResourceUserId, out);
}

Result hidsysGetNpadInterfaceType(HidNpadIdType id, u8 *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutU8(id, out, 316);
}

Result hidsysGetNpadLeftRightInterfaceType(HidNpadIdType id, u8 *out0, u8 *out1) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutU8U8(id, out0, out1, 317);
}

Result hidsysHasBattery(HidNpadIdType id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBool(id, out, 318);
}

Result hidsysHasLeftRightBattery(HidNpadIdType id, bool *out0, bool *out1) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp0=0, tmp1=0;
    Result rc = _hidsysCmdInU32OutU8U8(id, &tmp0, &tmp1, 319);
    if (R_SUCCEEDED(rc)) {
        if (out0) *out0 = tmp0 & 1;
        if (out1) *out1 = tmp1 & 1;
    }
    return rc;
}

Result hidsysGetUniquePadsFromNpad(HidNpadIdType id, HidsysUniquePadId *unique_pad_ids, s32 count, s32 *total_out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp=id;
    s64 out=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 321, tmp, out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { unique_pad_ids, count*sizeof(HidsysUniquePadId) } },
    );
    if (R_SUCCEEDED(rc) && total_out) *total_out = out;
    return rc;
}

Result hidsysEnableAppletToGetInput(bool enable) {
    const struct {
        u8 permitInput;
        u64 appletResourceUserId;
    } in = { enable!=0, g_hidsysAppletResourceUserId };

    return serviceDispatchIn(&g_hidsysSrv, 503, in);
}

Result hidsysGetUniquePadIds(HidsysUniquePadId *unique_pad_ids, s32 count, s32 *total_out) {
    s64 out=0;
    Result rc = serviceDispatchOut(&g_hidsysSrv, 703, out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { unique_pad_ids, count*sizeof(HidsysUniquePadId) } },
    );
    if (R_SUCCEEDED(rc) && total_out) *total_out = out;
    return rc;
}

Result hidsysGetUniquePadSerialNumber(HidsysUniquePadId unique_pad_id, HidsysUniquePadSerialNumber *serial) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_hidsysSrv, 809, unique_pad_id, *serial);
}

Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(7,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidsysNotificationLedPattern pattern;
        HidsysUniquePadId unique_pad_id;
    } in = { *pattern, unique_pad_id };

    return serviceDispatchIn(&g_hidsysSrv, 830, in);
}

Result hidsysSetNotificationLedPatternWithTimeout(const HidsysNotificationLedPattern *pattern, HidsysUniquePadId unique_pad_id, u64 timeout) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        HidsysNotificationLedPattern pattern;
        HidsysUniquePadId unique_pad_id;
        u64 timeout;
    } in = { *pattern, unique_pad_id, timeout };

    return serviceDispatchIn(&g_hidsysSrv, 831, in);
}

Result hidsysIsUsbFullKeyControllerEnabled(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoInOutBool(out, 850);
}

Result hidsysEnableUsbFullKeyController(bool flag) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBoolNoOut(flag, 851);
}

Result hidsysIsUsbConnected(HidsysUniquePadId unique_pad_id, bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id.id, out, 852);
}

Result hidsysIsFirmwareUpdateNeededForNotification(HidsysUniquePadId unique_pad_id, bool *out) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    s32 val=1;

    const struct {
        s32 val;
        u32 pad;
        HidsysUniquePadId unique_pad_id;
        u64 AppletResourceUserId;
    } in = { val, 0, unique_pad_id, appletGetAppletResourceUserId() };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&g_hidsysSrv, 1154, in, tmp,
        .in_send_pid = true,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result hidsysLegacyIsButtonConfigSupported(HidsysUniquePadId unique_pad_id, bool *out) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id.id, out, 1200);
}

Result hidsysIsButtonConfigSupported(BtdrvAddress addr, bool *out) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrOutBool(addr, out, 1200);
}

Result hidsysIsButtonConfigEmbeddedSupported(bool *out) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoInOutBool(out, 1201);
}

Result hidsysLegacyDeleteButtonConfig(HidsysUniquePadId unique_pad_id) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64NoOut(unique_pad_id.id, 1201);
}

Result hidsysDeleteButtonConfig(BtdrvAddress addr) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrNoOut(addr, 1202);
}

Result hidsysDeleteButtonConfigEmbedded(void) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoIO(1203);
}

Result hidsysLegacySetButtonConfigEnabled(HidsysUniquePadId unique_pad_id, bool flag) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBoolNoOut(unique_pad_id.id, flag, 1202);
}

Result hidsysSetButtonConfigEnabled(BtdrvAddress addr, bool flag) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrInBoolNoOut(addr, flag, 1204);
}

Result hidsysSetButtonConfigEmbeddedEnabled(bool flag) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInBoolNoOut(flag, 1205);
}

Result hidsysLegacyIsButtonConfigEnabled(HidsysUniquePadId unique_pad_id, bool *out) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id.id, out, 1203);
}

Result hidsysIsButtonConfigEnabled(BtdrvAddress addr, bool *out) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrInBoolNoOut(addr, out, 1206);
}

Result hidsysIsButtonConfigEmbeddedEnabled(bool *out) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoInOutBool(out, 1207);
}

Result hidsysLegacySetButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigEmbedded *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1204);
}

Result hidsysSetButtonConfigEmbedded(const HidsysButtonConfigEmbedded *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_hidsysSrv, 1208,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { config, sizeof(*config) } },
    );
}

Result hidsysLegacySetButtonConfigFull(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigFull *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1205);
}

Result hidsysSetButtonConfigFull(BtdrvAddress addr, const HidsysButtonConfigFull *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrInBufFixedNoOut(addr, config, sizeof(*config), 1209);
}

Result hidsysLegacySetButtonConfigLeft(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigLeft *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1206);
}

Result hidsysSetButtonConfigLeft(BtdrvAddress addr, const HidsysButtonConfigLeft *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrInBufFixedNoOut(addr, config, sizeof(*config), 12010);
}

Result hidsysLegacySetButtonConfigRight(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigRight *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1207);
}

Result hidsysSetButtonConfigRight(BtdrvAddress addr, const HidsysButtonConfigRight *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrInBufFixedNoOut(addr, config, sizeof(*config), 1211);
}

Result hidsysLegacyGetButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, HidsysButtonConfigEmbedded *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1208);
}

Result hidsysGetButtonConfigEmbedded(HidsysButtonConfigEmbedded *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatch(&g_hidsysSrv, 1212,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { config, sizeof(*config) } },
    );
}

Result hidsysLegacyGetButtonConfigFull(HidsysUniquePadId unique_pad_id, HidsysButtonConfigFull *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1209);
}

Result hidsysGetButtonConfigFull(BtdrvAddress addr, HidsysButtonConfigFull *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrOutBufFixedNoOut(addr, config, sizeof(*config), 1213);
}

Result hidsysLegacyGetButtonConfigLeft(HidsysUniquePadId unique_pad_id, HidsysButtonConfigLeft *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1210);
}

Result hidsysGetButtonConfigLeft(BtdrvAddress addr, HidsysButtonConfigLeft *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrOutBufFixedNoOut(addr, config, sizeof(*config), 1214);
}

Result hidsysLegacyGetButtonConfigRight(HidsysUniquePadId unique_pad_id, HidsysButtonConfigRight *config) {
    if (!hosversionBetween(10,11))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1211);
}

Result hidsysGetButtonConfigRight(BtdrvAddress addr, HidsysButtonConfigRight *config) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInAddrOutBufFixedNoOut(addr, config, sizeof(*config), 1215);
}

Result hidsysIsCustomButtonConfigSupported(HidsysUniquePadId unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id.id, out, 1250);
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

Result hidsysGetButtonConfigStorageEmbeddedDeprecated(s32 index, HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixed((u32)index, config, sizeof(*config), 1259);
}

Result hidsysGetButtonConfigStorageFullDeprecated(s32 index, HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixed((u32)index, config, sizeof(*config), 1260);
}

Result hidsysGetButtonConfigStorageLeftDeprecated(s32 index, HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixed((u32)index, config, sizeof(*config), 1261);
}

Result hidsysGetButtonConfigStorageRightDeprecated(s32 index, HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixed((u32)index, config, sizeof(*config), 1262);
}

Result hidsysSetButtonConfigStorageEmbeddedDeprecated(s32 index, const HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedNoOut((u32)index, config, sizeof(*config), 1263);
}

Result hidsysSetButtonConfigStorageFullDeprecated(s32 index, const HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedNoOut((u32)index, config, sizeof(*config), 1264);
}

Result hidsysSetButtonConfigStorageLeftDeprecated(s32 index, const HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedNoOut((u32)index, config, sizeof(*config), 1265);
}

Result hidsysSetButtonConfigStorageRightDeprecated(s32 index, const HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedNoOut((u32)index, config, sizeof(*config), 1266);
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

Result hidsysIsUsingCustomButtonConfig(HidsysUniquePadId unique_pad_id, bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBool(unique_pad_id.id, out, 1271);
}

Result hidsysIsAnyCustomButtonConfigEnabled(bool *out) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoInOutBool(out, 1272);
}

Result hidsysSetAllCustomButtonConfigEnabled(u64 AppletResourceUserId, bool flag) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBoolNoOut(AppletResourceUserId, flag, 1273);
}

Result hidsysSetDefaultButtonConfig(HidsysUniquePadId unique_pad_id) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64NoOut(unique_pad_id.id, 1274);
}

Result hidsysSetAllDefaultButtonConfig(void) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdNoIO(1275);
}

Result hidsysSetHidButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1276);
}

Result hidsysSetHidButtonConfigFull(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1277);
}

Result hidsysSetHidButtonConfigLeft(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1278);
}

Result hidsysSetHidButtonConfigRight(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64InBufFixedNoOut(unique_pad_id.id, config, sizeof(*config), 1279);
}

Result hidsysGetHidButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigEmbedded *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1280);
}

Result hidsysGetHidButtonConfigFull(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigFull *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1281);
}

Result hidsysGetHidButtonConfigLeft(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigLeft *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1282);
}

Result hidsysGetHidButtonConfigRight(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigRight *config) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU64OutBufFixed(unique_pad_id.id, config, sizeof(*config), 1283);
}

Result hidsysGetButtonConfigStorageEmbedded(s32 index, HidcfgButtonConfigEmbedded *config, HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixedOutPtrFixed((u32)index, config, sizeof(*config), name, sizeof(*name), 1284);
}

Result hidsysGetButtonConfigStorageFull(s32 index, HidcfgButtonConfigFull *config, HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixedOutPtrFixed((u32)index, config, sizeof(*config), name, sizeof(*name), 1285);
}

Result hidsysGetButtonConfigStorageLeft(s32 index, HidcfgButtonConfigLeft *config, HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixedOutPtrFixed((u32)index, config, sizeof(*config), name, sizeof(*name), 1286);
}

Result hidsysGetButtonConfigStorageRight(s32 index, HidcfgButtonConfigRight *config, HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32OutBufFixedOutPtrFixed((u32)index, config, sizeof(*config), name, sizeof(*name), 1287);
}

Result hidsysSetButtonConfigStorageEmbedded(s32 index, const HidcfgButtonConfigEmbedded *config, const HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedInPtrFixedNoOut((u32)index, config, sizeof(*config), name, sizeof(*name), 1288);
}

Result hidsysSetButtonConfigStorageFull(s32 index, const HidcfgButtonConfigFull *config, const HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedInPtrFixedNoOut((u32)index, config, sizeof(*config), name, sizeof(*name), 1289);
}

Result hidsysSetButtonConfigStorageLeft(s32 index, const HidcfgButtonConfigLeft *config, const HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedInPtrFixedNoOut((u32)index, config, sizeof(*config), name, sizeof(*name), 1290);
}

Result hidsysSetButtonConfigStorageRight(s32 index, const HidcfgButtonConfigRight *config, const HidcfgStorageName *name) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _hidsysCmdInU32InBufFixedInPtrFixedNoOut((u32)index, config, sizeof(*config), name, sizeof(*name), 1291);
}

