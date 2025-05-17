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
static Service g_nfcMfSrv;
static Service g_nfcMfInterface;

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
static Result _nfcInterfaceInitialize(Service* srv, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id);

static Result _nfcCmdNoIO(Service* srv, u32 cmd_id);
static Result _nfcCmdInDevhandleNoOut(Service* srv, const NfcDeviceHandle *handle, u32 cmd_id);
static Result _nfcCmdInDevhandleOutEvent(Service* srv, const NfcDeviceHandle *handle, Event *out_event, u32 cmd_id);
static Result _nfcCmdInDevhandleOutBuffer(Service* srv, const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 cmd_id);
static Result _nfcCmdInDevhandleInBufferNoOut(Service* srv, const NfcDeviceHandle *handle, const void* buf, size_t buf_size, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD_PARAMS(nfp, (NfpServiceType service_type), (service_type));

Result _nfpInitialize(NfpServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

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
        rc = _nfcInterfaceInitialize(&g_nfpInterface, g_nfcVersionData, 2, 0);

    return rc;
}

void _nfpCleanup(void) {
    _nfcCmdNoIO(&g_nfpInterface, 1); // Finalize
    serviceClose(&g_nfpInterface);
    serviceClose(&g_nfpSrv);
}

NX_GENERATE_SERVICE_GUARD_PARAMS(nfc, (NfcServiceType service_type), (service_type));

Result _nfcInitialize(NfcServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

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
        rc = _nfcInterfaceInitialize(&g_nfcInterface, g_nfcVersionData, 2, hosversionBefore(4,0,0) ? 0 : 400);

    return rc;
}

void _nfcCleanup(void) {
    _nfcCmdNoIO(&g_nfcInterface, hosversionBefore(4,0,0) ? 1 : 401); // Finalize
    serviceClose(&g_nfcInterface);
    serviceClose(&g_nfcSrv);
}

NX_GENERATE_SERVICE_GUARD(nfcMf);

Result _nfcMfInitialize() {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);

    rc = smGetService(&g_nfcMfSrv, "nfc:mf:u");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfcMfSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfcCreateInterface(&g_nfcMfSrv, &g_nfcMfInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfcInterfaceInitialize(&g_nfcMfInterface, g_nfcVersionData, 2, 0);

    return rc;
}

void _nfcMfCleanup(void) {
    _nfcCmdNoIO(&g_nfcMfInterface, 1); // Finalize
    serviceClose(&g_nfcMfInterface);
    serviceClose(&g_nfcMfSrv);
}

Service* nfpGetServiceSession(void) {
    return &g_nfpSrv;
}

Service* nfpGetServiceSession_Interface(void) {
    return &g_nfpInterface;
}

Service* nfcGetServiceSession(void) {
    return &g_nfcSrv;
}

Service* nfcGetServiceSession_Interface(void) {
    return &g_nfcInterface;
}

Service* nfcMfGetServiceSession(void) {
    return &g_nfcMfSrv;
}

Service* nfcMfGetServiceSession_Interface(void) {
    return &g_nfcMfInterface;
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

static Result _nfcCmdInDevhandleInBufferNoOut(Service* srv, const NfcDeviceHandle *handle, const void* buf, size_t buf_size, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, *handle,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

static Result _nfcInterfaceInitialize(Service* srv, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id) {
    const struct {
        u64 aruid;
        u64 zero;
    } in = { appletGetAppletResourceUserId(), 0 };

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

Result nfcListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    serviceAssumeDomain(&g_nfcInterface);
    return serviceDispatchOut(&g_nfcInterface, 404, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, count*sizeof(NfcDeviceHandle) } },
    );
}

Result nfcMfListDevices(s32 *total_out, NfcDeviceHandle *out, s32 count) {
    serviceAssumeDomain(&g_nfcMfInterface);
    return serviceDispatchOut(&g_nfcMfInterface, 2, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { out, count*sizeof(NfcDeviceHandle) } },
    );
}

Result nfpStartDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 3);
}

Result nfcStartDetection(const NfcDeviceHandle *handle, NfcProtocol protocol) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        NfcDeviceHandle handle;
        NfcProtocol protocol;
    } in = { *handle, protocol };

    serviceAssumeDomain(&g_nfcInterface);
    return serviceDispatchIn(&g_nfcInterface, 408, in);
}

Result nfcMfStartDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfcMfInterface, handle, 3);
}

Result nfpStopDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 4);
}

Result nfcStopDetection(const NfcDeviceHandle *handle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 409);
}

Result nfcMfStopDetection(const NfcDeviceHandle *handle) {
    return _nfcCmdInDevhandleNoOut(&g_nfcMfInterface, handle, 4);
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

Result nfpOpenApplicationArea(const NfcDeviceHandle *handle, u32 app_id) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        NfcDeviceHandle handle;
        u32 app_id;
    } in = { *handle, app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 7, in);
}

Result nfpGetApplicationArea(const NfcDeviceHandle *handle, void* buf, size_t buf_size, u32 *out_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchInOut(&g_nfpInterface, 8, *handle, *out_size,
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

Result nfpRecreateApplicationArea(const NfcDeviceHandle *handle, u32 app_id, const void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        NfcDeviceHandle handle;
        u32 app_id;
    } in = { *handle, app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 24, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpGetApplicationAreaSize(const NfcDeviceHandle *handle, u32 *out_app_area_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, out_app_area_size, 22);
}

Result nfpDeleteApplicationArea(const NfcDeviceHandle *handle) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleNoOut(&g_nfpInterface, handle, 105);
}

Result nfpExistsApplicationArea(const NfcDeviceHandle *handle, bool *out) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    u32 tmp=0;
    Result rc = _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, &tmp, 106);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetTagInfo(const NfcDeviceHandle *handle, NfpTagInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpTagInfo), 13);
}

Result nfcGetTagInfo(const NfcDeviceHandle *handle, NfcTagInfo *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    return _nfcCmdInDevhandleOutBuffer(&g_nfcInterface, handle, out, sizeof(NfcTagInfo), 410);
}

Result nfcMfGetTagInfo(const NfcDeviceHandle *handle, NfcTagInfo *out) {
    return _nfcCmdInDevhandleOutBuffer(&g_nfcMfInterface, handle, out, sizeof(NfcTagInfo), 7);
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

Result nfpGetAdminInfo(const NfcDeviceHandle *handle, NfpAdminInfo *out) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpAdminInfo), 101);
}

Result nfpAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfpInterface, handle, out_event, 17);
}

Result nfcAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    return _nfcCmdInDevhandleOutEvent(&g_nfcInterface, handle, out_event, 411);
}

Result nfcMfAttachActivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfcMfInterface, handle, out_event, 8);
}

Result nfpAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfpInterface, handle, out_event, 18);
}

Result nfcAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    return _nfcCmdInDevhandleOutEvent(&g_nfcInterface, handle, out_event, 412);
}

Result nfcMfAttachDeactivateEvent(const NfcDeviceHandle *handle, Event *out_event) {
    return _nfcCmdInDevhandleOutEvent(&g_nfcMfInterface, handle, out_event, 9);
}

Result nfpGetState(NfcState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdNoInOutU32(&g_nfpInterface, &tmp, 19);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfcGetState(NfcState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdNoInOutU32(&g_nfcInterface, &tmp, hosversionBefore(4,0,0) ? 2 : 402);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfcMfGetState(NfcState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdNoInOutU32(&g_nfcMfInterface, &tmp, 10);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetDeviceState(const NfcDeviceHandle *handle, NfpDeviceState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, &tmp, 20);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfcGetDeviceState(const NfcDeviceHandle *handle, NfcDeviceState *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    u32 tmp=0;
    Result rc = _nfcCmdInDevhandleOutU32(&g_nfcInterface, handle, &tmp, 405);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfcMfGetDeviceState(const NfcDeviceHandle *handle, NfcMifareDeviceState *out) {
    u32 tmp=0;
    Result rc = _nfcCmdInDevhandleOutU32(&g_nfcMfInterface, handle, &tmp, 11);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetNpadId(const NfcDeviceHandle *handle, u32 *out) {
    return _nfcCmdInDevhandleOutU32(&g_nfpInterface, handle, out, 21);
}

Result nfcGetNpadId(const NfcDeviceHandle *handle, u32 *out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    return _nfcCmdInDevhandleOutU32(&g_nfcInterface, handle, out, 406);
}

Result nfcMfGetNpadId(const NfcDeviceHandle *handle, u32 *out) {
    return _nfcCmdInDevhandleOutU32(&g_nfcMfInterface, handle, out, 12);
}

Result nfpAttachAvailabilityChangeEvent(Event *out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nfcCmdGetEvent(&g_nfpInterface, out_event, true, 23);
}

Result nfcAttachAvailabilityChangeEvent(Event *out_event) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nfcCmdGetEvent(&g_nfcInterface, out_event, true, 407);
}

Result nfcMfAttachAvailabilityChangeEvent(Event *out_event) {
    return _nfcCmdGetEvent(&g_nfcMfInterface, out_event, true, 13);
}

Result nfcFormat(const NfcDeviceHandle *handle) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 100);
}

Result nfpGetRegisterInfoPrivate(const NfcDeviceHandle *handle, NfpRegisterInfoPrivate *out) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpRegisterInfoPrivate), 102);
}

Result nfpSetRegisterInfoPrivate(const NfcDeviceHandle *handle, const NfpRegisterInfoPrivate *register_info_private) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleInBufferNoOut(&g_nfcInterface, handle, register_info_private, sizeof(NfpRegisterInfoPrivate) , 103);
}

Result nfpDeleteRegisterInfo(const NfcDeviceHandle *handle) {
    if (g_nfpServiceType == NfpServiceType_User)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 104);
}

Result nfpGetAll(const NfcDeviceHandle *handle, NfpData *out) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleOutBuffer(&g_nfpInterface, handle, out, sizeof(NfpData), 200);
}

Result nfpSetAll(const NfcDeviceHandle *handle, const NfpData *nfp_data) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleInBufferNoOut(&g_nfcInterface, handle, nfp_data, sizeof(NfpData) , 201);
}

Result nfpFlushDebug(const NfcDeviceHandle *handle) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 202);
}

Result nfpBreakTag(const NfcDeviceHandle *handle, NfpBreakType break_type) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    const struct {
        NfcDeviceHandle handle;
        u32 break_type;
    } in = { *handle, break_type };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 203, in);
}

Result nfpReadBackupData(const NfcDeviceHandle *handle, void* out_buf, size_t buf_size, u32 *out_size) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchInOut(&g_nfpInterface, 204, *handle, *out_size,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out_buf, buf_size } },
    );
}

Result nfpWriteBackupData(const NfcDeviceHandle *handle, const void* buf, size_t buf_size) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 205, *handle,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

/// Only available with ::NfpServiceType_Debug.
Result nfpWriteNtf(const NfcDeviceHandle *handle, u32 write_type, const void* buf, size_t buf_size) {
    if (g_nfpServiceType != NfpServiceType_Debug)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
    
    const struct {
        NfcDeviceHandle handle;
        u32 write_type;
    } in = { *handle, write_type };


    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 206, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfcIsNfcEnabled(bool *out) {
    return _nfcCmdNoInOutBool(&g_nfcInterface, out, hosversionBefore(4,0,0) ? 3 : 403);
}

Result nfcReadMifare(const NfcDeviceHandle *handle, NfcMifareReadBlockData *out_block_data, const NfcMifareReadBlockParameter *read_block_parameter, s32 count) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    serviceAssumeDomain(&g_nfcInterface);
    Result rc = serviceDispatchIn(&g_nfcInterface, 1000, *handle,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { out_block_data, count*sizeof(NfcMifareReadBlockData) },
            { read_block_parameter, count*sizeof(NfcMifareReadBlockParameter) },
        },
    );
    return rc;
}

Result nfcMfReadMifare(const NfcDeviceHandle *handle, NfcMifareReadBlockData *out_block_data, const NfcMifareReadBlockParameter *read_block_parameter, s32 count) {
    serviceAssumeDomain(&g_nfcMfInterface);
    Result rc = serviceDispatchIn(&g_nfcMfInterface, 5, *handle,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { out_block_data, count*sizeof(NfcMifareReadBlockData) },
            { read_block_parameter, count*sizeof(NfcMifareReadBlockParameter) },
        },
    );
    return rc;
}

Result nfcWriteMifare(const NfcDeviceHandle *handle, const NfcMifareWriteBlockParameter *write_block_parameter, s32 count) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    
    serviceAssumeDomain(&g_nfcInterface);
    Result rc = serviceDispatchIn(&g_nfcInterface, 1001, *handle,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { write_block_parameter, count*sizeof(NfcMifareWriteBlockParameter) },
        },
    );
    return rc;
}

Result nfcMfWriteMifare(const NfcDeviceHandle *handle, const NfcMifareWriteBlockParameter *write_block_parameter, s32 count) {
    serviceAssumeDomain(&g_nfcMfInterface);
    Result rc = serviceDispatchIn(&g_nfcMfInterface, 6, *handle,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { write_block_parameter, count*sizeof(NfcMifareWriteBlockParameter) },
        },
    );
    return rc;
}

Result nfcSendCommandByPassThrough(const NfcDeviceHandle *handle, u64 timeout, const void* cmd_buf, size_t cmd_buf_size, void* reply_buf, size_t reply_buf_size, u64 *out_size) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        NfcDeviceHandle handle;
        u64 timeout;
    } in = { *handle, timeout };

    serviceAssumeDomain(&g_nfcInterface);
    u32 tmp_out=0;
    Result rc = serviceDispatchInOut(&g_nfcInterface, 1300, in, tmp_out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias | SfBufferAttr_Out,
            SfBufferAttr_HipcMapAlias | SfBufferAttr_In,
        },
        .buffers = {
            { reply_buf, reply_buf_size },
            { cmd_buf, cmd_buf_size },
        },
    );
    if (R_SUCCEEDED(rc) && out_size) *out_size = tmp_out;
    return rc;
}

Result nfcKeepPassThroughSession(const NfcDeviceHandle *handle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 1301);
}

Result nfcReleasePassThroughSession(const NfcDeviceHandle *handle) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _nfcCmdInDevhandleNoOut(&g_nfcInterface, handle, 1302);
}
