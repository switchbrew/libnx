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
    
    rc = appletGetAppletResourceUserId(&g_hidsysAppletResourceUserId);
    if (R_FAILED(rc))
        g_hidsysAppletResourceUserId = 0;

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
