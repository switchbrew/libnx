#include "service_guard.h"
#include <string.h>
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/nfc.h"

static NfpServiceType g_nfpServiceType;
static NfcServiceType g_nfcServiceType;
static Service g_nfpSrv;
static Service g_nfpInterface;
static Service g_nfcSrv;
static Service g_nfcInterface;

static const NfcRequiredMcuVersionData g_nfcVersionData[2] = {
    {
        .version = 0x00000001000a0003,
        .reserved = {0},
    },
    {
        .version = 0x0000000300040003,
        .reserved = {0},
    },
};

static Result _nfcCreateInterface(Service* srv, Service* srv_out);
static Result _nfcInterfaceInitialize(Service* srv, u64 aruid, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id);

static Result _nfcCmdNoIO(Service* srv, u32 cmd_id);
static Result _nfcCmdInDevhandleNoOut(Service* srv, const NfcDeviceHandle *handle, u32 cmd_id);
static Result _nfcCmdInDevhandleOutEvent(Service* srv, const NfcDeviceHandle *handle, Event *out_event, u32 cmd_id);
static Result _nfcCmdInDevhandleOutBuffer(Service* srv, const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD_PARAMS(nfp, (NfpServiceType service_type), (service_type));

Result _nfpInitialize(NfpServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u64 aruid = 0;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    appletGetAppletResourceUserId(&aruid);

    g_nfpServiceType = service_type;
    switch (g_nfpServiceType) {
        case NfpServiceType_User:
            rc = smGetService(&g_nfpSrv, "nfp:user");
            break;
        case NfpServiceType_Debug:
            rc = smGetService(&g_nfpSrv, "nfp:dbg");
            break;
        case NfpServiceType_System:
            rc = smGetService(&g_nfpSrv, "nfp:sys");
            break;
    }

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfpSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfcCreateInterface(&g_nfpSrv, &g_nfpInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfcInterfaceInitialize(&g_nfpInterface, aruid, g_nfcVersionData, 2, 0);

    return rc;
}

void _nfpCleanup(void) {
    _nfcCmdNoIO(&g_nfpInterface, 1); // Finalize
    serviceClose(&g_nfpInterface);
    serviceClose(&g_nfpSrv);
}

NX_GENERATE_SERVICE_GUARD_PARAMS(nfc, (NfcServiceType service_type), (service_type));

Result _nfcInitialize(NfcServiceType service_type) {
    Result rc=0;
    u64 aruid = 0;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    appletGetAppletResourceUserId(&aruid);

    g_nfcServiceType = service_type;
    switch (g_nfcServiceType) {
        case NfcServiceType_User:
            rc = smGetService(&g_nfcSrv, "nfc:user");
            break;
        case NfcServiceType_System:
            rc = smGetService(&g_nfcSrv, "nfc:sys");
            break;
    }

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfcSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfcCreateInterface(&g_nfcSrv, &g_nfcInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfcInterfaceInitialize(&g_nfcInterface, aruid, g_nfcVersionData, 2, hosversionBefore(4,0,0) ? 0 : 400);

    return rc;
}

void _nfcCleanup(void) {
    _nfcCmdNoIO(&g_nfcInterface, hosversionBefore(4,0,0) ? 1 : 401); // Finalize
    serviceClose(&g_nfcInterface);
    serviceClose(&g_nfcSrv);
}

Service* nfpGetServiceSession(void) {
    return &g_nfpSrv;
}

Service* nfpGetServiceSession_Interface(void) {
    return &g_nfpInterface;
}

Service* nfcGetServiceSession(void) {
    return &g_nfpSrv;
}

Service* nfcGetServiceSession_Interface(void) {
    return &g_nfpInterface;
}

static Result _nfcCreateInterface(Service* srv, Service* srv_out) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, 0,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _nfcCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _nfcCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _nfcCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _nfcCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _nfcCmdInDevhandleNoOut(Service* srv, const NfcDeviceHandle *handle, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, *handle);
}

static Result _nfcCmdInDevhandleOutU32(Service* srv, const NfcDeviceHandle *handle, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchInOut(srv, cmd_id, *handle, *out);
}

static Result _nfcCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfcCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfcCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _nfcCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _nfcCmdInDevhandleOutEvent(Service* srv, const NfcDeviceHandle *handle, Event *out_event, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    serviceAssumeDomain(srv);
    Result rc = serviceDispatchIn(srv, cmd_id, *handle,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, false);
    return rc;
}

static Result _nfcCmdInDevhandleOutBuffer(Service* srv, const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, *handle,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

static Result _nfcInterfaceInitialize(Service* srv, u64 aruid, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id) {
    const struct {
        u64 aruid;
        u64 zero;
    } in = { aruid, 0 };

    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { version, count*sizeof(NfcRequiredMcuVersionData) } },
        .in_send_pid = true,
    );
}

Result nfpListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count) {
    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchOut(&g_nfpInterface, 2, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, count*sizeof(NfcDeviceHandle) } },
    );
}

Result nfpStartDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 3);
}

Result nfpStopDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 4);
}

Result nfpMount(const NfcDeviceHandle *handle, NfpDeviceType device_type, NfpMountTarget mount_target) {
    const struct {
        NfcDeviceHandle handle;
        u32 device_type;
        u32 mount_target;
    } in = { *handle, device_type, mount_target };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 5, in);
}

Result nfpUnmount(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 6);
}

Result nfpOpenApplicationArea(const NfcDeviceHandle *handle, u32 app_id, u32 *npad_id) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        NfcDeviceHandle handle;
        u32 app_id;
    } in = { *handle, app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchInOut(&g_nfpInterface, 7, in, *npad_id);
}

Result nfpGetApplicationArea(const NfcDeviceHandle *handle, void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 8, *handle,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpSetApplicationArea(const NfcDeviceHandle *handle, const void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 9, *handle,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpFlush(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 10);
}

Result nfpRestore(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 11);
}

Result nfpCreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        NfcDeviceHandle handle;
        u32 app_id;
    } in = { *handle, app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 12, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpGetTagInfo(const NfcDeviceHandle *handle, NfpTagInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpTagInfo), 13);
}

Result nfpGetRegisterInfo(const NfcDeviceHandle *handle, NfpRegisterInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpRegisterInfo), 14);
}

Result nfpGetCommonInfo(const NfcDeviceHandle *handle, NfpCommonInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpCommonInfo), 15);
}

Result nfpGetModelInfo(const NfcDeviceHandle *handle, NfpModelInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpModelInfo), 16);
}

Result nfpAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfpInterface, handle, out_event, 17);
}

Result nfpAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfpInterface, handle, out_event, 18);
}

Result nfpGetState(NfpState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdNoInOutU32(&g_nfpInterface, &tmp, 19);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetDeviceState(const NfcDeviceHandle *handle, NfpDeviceState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, &tmp, 20);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetNpadId(const NfcDeviceHandle *handle, u32 *out) {
    return _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, out, 21);
}

Result nfpAttachAvailabilityChangeEvent(Event *out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nfcCmdGetEvent(&g_nfpInterface, out_event, true, 23);
}

Result nfcIsNfcEnabled(bool *out) {
    return _nfcCmdNoInOutBool(&g_nfcInterface, out, hosversionBefore(4,0,0) ? 3 : 403);
}
