#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/btmsys.h"

static Service g_btmIBtmSystemCore;

static Result _btmsysGetSession(Service* srv, Service* srv_out, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(btmsys);

Result _btmsysInitialize(void) {
    Result rc=0;
    Service srv={0};

    rc = btmsysGetServiceSession(&srv);
    if (R_SUCCEEDED(rc)) rc = _btmsysGetSession(&srv, &g_btmIBtmSystemCore, 0); // GetCore
    serviceClose(&srv);
    return rc;
}

void _btmsysCleanup(void) {
    serviceClose(&g_btmIBtmSystemCore);
}

Result btmsysGetServiceSession(Service* srv_out) {
    return smGetService(srv_out, "btm:sys");
}

Service* btmsysGetServiceSession_IBtmSystemCore(void) {
    return &g_btmIBtmSystemCore;
}

static Result _btmsysGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _btmsysCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_btmIBtmSystemCore, cmd_id);
}

static Result _btmsysCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmIBtmSystemCore, cmd_id, *out);
}

static Result _btmsysCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _btmsysCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _btmsysCmdGetEvent(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = serviceDispatch(&g_btmIBtmSystemCore, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btmsysCmdGetEventOutFlag(Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;
    u8 out=0;

    rc = serviceDispatchOut(&g_btmIBtmSystemCore, cmd_id, out,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc) && !out) rc = MAKERESULT(Module_Libnx, LibnxError_ShouldNotHappen);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _btmsysCmdOutBufPtr(void* buffer, size_t size, s32 *total_out, u32 cmd_id) {
    return serviceDispatchOut(&g_btmIBtmSystemCore, cmd_id, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { {buffer, size} },
    );
}

static Result _btmsysCmdInAddrNoOut(BtdrvAddress addr, u32 cmd_id) {
    return serviceDispatchIn(&g_btmIBtmSystemCore, cmd_id, addr);
}

static Result _btmsysCmdAruidNoOutput(u32 cmd_id) {
    u64 AppletResourceUserId = appletGetAppletResourceUserId();
    return serviceDispatchIn(&g_btmIBtmSystemCore, cmd_id, AppletResourceUserId,
        .in_send_pid = true,
    );
}

Result btmsysStartGamepadPairing(void) {
    return _btmsysCmdNoIO(0);
}

Result btmsysCancelGamepadPairing(void) {
    return _btmsysCmdNoIO(1);
}

Result btmsysClearGamepadPairingDatabase(void) {
    return _btmsysCmdNoIO(2);
}

Result btmsysGetPairedGamepadCount(u8 *out) {
    return _btmsysCmdNoInOutU8(out, 3);
}

Result btmsysEnableRadio(void) {
    return _btmsysCmdNoIO(4);
}

Result btmsysDisableRadio(void) {
    return _btmsysCmdNoIO(5);
}

Result btmsysGetRadioOnOff(bool *out) {
    return _btmsysCmdNoInOutBool(out, 6);
}

Result btmsysAcquireRadioEvent(Event* out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdGetEventOutFlag(out_event, true, 7);
}

Result btmsysAcquireGamepadPairingEvent(Event* out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdGetEventOutFlag(out_event, true, 8);
}

Result btmsysIsGamepadPairingStarted(bool *out) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdNoInOutBool(out, 9);
}

Result btmsysStartAudioDeviceDiscovery(void) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdNoIO(10);
}

Result btmsysStopAudioDeviceDiscovery(void) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdNoIO(11);
}

Result btmsysIsDiscoveryingAudioDevice(bool *out) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdNoInOutBool(out, 12);
}

Result btmsysGetDiscoveredAudioDevice(BtmAudioDevice *out, s32 count, s32 *total_out) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdOutBufPtr(out, sizeof(BtmAudioDevice)*count, total_out, 13);
}

Result btmsysAcquireAudioDeviceConnectionEvent(Event* out_event) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdGetEvent(out_event, true, 14);
}

Result btmsysConnectAudioDevice(BtdrvAddress addr) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdInAddrNoOut(addr, 15);
}

Result btmsysIsConnectingAudioDevice(bool *out) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdNoInOutBool(out, 16);
}

Result btmsysGetConnectedAudioDevices(BtmAudioDevice *out, s32 count, s32 *total_out) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdOutBufPtr(out, sizeof(BtmAudioDevice)*count, total_out, 17);
}

Result btmsysDisconnectAudioDevice(BtdrvAddress addr) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdInAddrNoOut(addr, 18);
}

Result btmsysAcquirePairedAudioDeviceInfoChangedEvent(Event* out_event) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdGetEvent(out_event, true, 19);
}

Result btmsysGetPairedAudioDevices(BtmAudioDevice *out, s32 count, s32 *total_out) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdOutBufPtr(out, sizeof(BtmAudioDevice)*count, total_out, 20);
}

Result btmsysRemoveAudioDevicePairing(BtdrvAddress addr) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdInAddrNoOut(addr, 21);
}

Result btmsysRequestAudioDeviceConnectionRejection(void) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdAruidNoOutput(22);
}

Result btmsysCancelAudioDeviceConnectionRejection(void) {
    if (hosversionBefore(13,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _btmsysCmdAruidNoOutput(23);
}
