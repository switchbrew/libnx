#include "service_guard.h"
#include <string.h>
#include "runtime/hosversion.h"
#include "services/hid.h"
#include "services/applet.h"
#include "services/nfc.h"

static NfpServiceType g_nfpServiceType = NfpServiceType_NotInitialized;
static NfcServiceType g_nfcServiceType = NfcServiceType_NotInitialized;
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

static Result _nfpCreateInterface(Service* srv, Service* srv_out);
static Result _nfpInterfaceInitialize(Service* srv, u64 aruid, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id);
static Result _nfpInterfaceFinalize(Service* srv, u32 cmd_id);

static Result _nfpCmdNoIO(Service* srv, u32 cmd_id);
static Result _nfpCmdInIdNoOut(Service* srv, HidControllerID id, u32 cmd_id);
static Result _nfpCmdInIdOutEvent(Service* srv, HidControllerID id, Event *out_event, u32 cmd_id);
static Result _nfpCmdInIdOutBuffer(Service* srv, HidControllerID id, void* buf, size_t buf_size, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(nfp);

void nfpSetServiceType(NfpServiceType serviceType) {
    g_nfpServiceType = serviceType;
}

void nfcSetServiceType(NfcServiceType serviceType) {
    g_nfcServiceType = serviceType;
}

Result _nfpInitialize(void) {
    Result rc=0;
    u64 aruid = 0;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    appletGetAppletResourceUserId(&aruid);

    switch (g_nfpServiceType) {
        case NfpServiceType_NotInitialized:
        case NfpServiceType_User:
            g_nfpServiceType = NfpServiceType_User;
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
        rc = _nfpCreateInterface(&g_nfpSrv, &g_nfpInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpInterfaceInitialize(&g_nfpInterface, aruid, g_nfcVersionData, 2, 0);

    return rc;
}

void _nfpCleanup(void) {
    _nfpInterfaceFinalize(&g_nfpInterface, 1);
    serviceClose(&g_nfpInterface);
    serviceClose(&g_nfpSrv);
    g_nfpServiceType = NfpServiceType_NotInitialized;
}

NX_GENERATE_SERVICE_GUARD(nfc);

Result _nfcInitialize(void) {
    Result rc=0;
    u64 aruid = 0;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    appletGetAppletResourceUserId(&aruid);

    switch (g_nfcServiceType) {
        case NfcServiceType_NotInitialized:
        case NfcServiceType_User:
            g_nfcServiceType = NfcServiceType_User;
            rc = smGetService(&g_nfcSrv, "nfc:user");
            break;
        case NfcServiceType_System:
            rc = smGetService(&g_nfcSrv, "nfc:sys");
            break;
    }

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfcSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpCreateInterface(&g_nfcSrv, &g_nfcInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpInterfaceInitialize(&g_nfcInterface, aruid, g_nfcVersionData, 2, hosversionBefore(4,0,0) ? 0 : 400);

    return rc;
}

void _nfcCleanup(void) {
    _nfpInterfaceFinalize(&g_nfcInterface, hosversionBefore(4,0,0) ? 1 : 401);
    serviceClose(&g_nfcInterface);
    serviceClose(&g_nfcSrv);
    g_nfcServiceType = NfcServiceType_NotInitialized;
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

static Result _nfpCreateInterface(Service* srv, Service* srv_out) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, 0,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _nfpCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _fpuCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _nfpCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _nfpCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _nfpCmdInIdNoOut(Service* srv, HidControllerID id, u32 cmd_id) {
    serviceAssumeDomain(srv);
    u64 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchIn(srv, cmd_id, tmp);
}

static Result _nfpCmdInIdOutU32(Service* srv, HidControllerID id, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    u64 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchInOut(srv, cmd_id, tmp, *out);
}

static Result _nfpCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfpCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfpCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _nfpCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp!=0;
    return rc;
}

static Result _nfpCmdInIdOutEvent(Service* srv, HidControllerID id, Event *out_event, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(srv);
    Result rc = serviceDispatchIn(srv, cmd_id, tmp,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, false);
    return rc;
}

static Result _nfpCmdInIdOutBuffer(Service* srv, HidControllerID id, void* buf, size_t buf_size, u32 cmd_id) {
    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, tmp,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

static Result _nfpInterfaceInitialize(Service* srv, u64 aruid, const NfcRequiredMcuVersionData *version, s32 count, u32 cmd_id) {
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

static Result _nfpInterfaceFinalize(Service* srv, u32 cmd_id) {
    return _nfpCmdNoIO(srv, cmd_id);
}

Result nfpListDevices(s32 *count, HidControllerID *out, size_t num_elements) {
    // This is the maximum number of controllers that can be connected to a console at a time
    // Incidentally, this is the biggest value official software (SSBU) was observed using
    size_t max_controllers = 9;
    if (num_elements > max_controllers)
        num_elements = max_controllers;

    u64 buf[max_controllers];
    memset(buf, 0, sizeof(buf));

    serviceAssumeDomain(&g_nfpInterface);
    Result rc = serviceDispatchOut(&g_nfpInterface, 2, *count,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buf, max_controllers*sizeof(u64) } },
    );
    if (R_SUCCEEDED(rc) && out) {
        for (size_t i=0; i<num_elements; i++)
            out[i] = hidControllerIDFromOfficial(buf[i]);
    }
    return rc;
}

Result nfpStartDetection(HidControllerID id) {
    return _nfpCmdInIdNoOut(&g_nfpInterface, id, 3);
}

Result nfpStopDetection(HidControllerID id) {
    return _nfpCmdInIdNoOut(&g_nfpInterface, id, 4);
}

Result nfpMount(HidControllerID id, NfpDeviceType device_type, NfpMountTarget mount_target) {
    const struct {
        u64 id;
        u32 device_type;
        u32 mount_target;
    } in = { hidControllerIDToOfficial(id), device_type, mount_target };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 5, in);
}

Result nfpUnmount(HidControllerID id) {
    return _nfpCmdInIdNoOut(&g_nfpInterface, id, 6);
}

Result nfpOpenApplicationArea(HidControllerID id, u32 app_id, u32 *npad_id) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u64 id;
        u32 app_id;
    } in = { hidControllerIDToOfficial(id), app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchInOut(&g_nfpInterface, 7, in, *npad_id);
}

Result nfpGetApplicationArea(HidControllerID id, void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 8, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpSetApplicationArea(HidControllerID id, const void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 9, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpFlush(HidControllerID id) {
    return _nfpCmdInIdNoOut(&g_nfpInterface, id, 10);
}

Result nfpRestore(HidControllerID id) {
    return _nfpCmdInIdNoOut(&g_nfpInterface, id, 11);
}

Result nfpCreateApplicationArea(HidControllerID id, u32 app_id, const void* buf, size_t buf_size) {
    if (g_nfpServiceType == NfpServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    const struct {
        u64 id;
        u32 app_id;
    } PACKED in = { hidControllerIDToOfficial(id), app_id };

    serviceAssumeDomain(&g_nfpInterface);
    return serviceDispatchIn(&g_nfpInterface, 12, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpGetTagInfo(HidControllerID id, NfpTagInfo *out) {
    return _nfpCmdInIdOutBuffer(&g_nfpInterface, id, out, sizeof(NfpTagInfo), 13);
}

Result nfpGetRegisterInfo(HidControllerID id, NfpRegisterInfo *out) {
    return _nfpCmdInIdOutBuffer(&g_nfpInterface, id, out, sizeof(NfpRegisterInfo), 14);
}

Result nfpGetCommonInfo(HidControllerID id, NfpCommonInfo *out) {
    return _nfpCmdInIdOutBuffer(&g_nfpInterface, id, out, sizeof(NfpCommonInfo), 15);
}

Result nfpGetModelInfo(HidControllerID id, NfpModelInfo *out) {
    return _nfpCmdInIdOutBuffer(&g_nfpInterface, id, out, sizeof(NfpModelInfo), 16);
}

Result nfpAttachActivateEvent(HidControllerID id, Event *out_event) {
    return _nfpCmdInIdOutEvent(&g_nfpInterface, id, out_event, 17);
}

Result nfpAttachDeactivateEvent(HidControllerID id, Event *out_event) {
    return _nfpCmdInIdOutEvent(&g_nfpInterface, id, out_event, 18);
}

Result nfpGetState(NfpState *out) {
    u32 tmp=0;
    Result rc = _nfpCmdNoInOutU32(&g_nfpInterface, &tmp, 19);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetDeviceState(HidControllerID id, NfpDeviceState *out) {
    u32 tmp=0;
    Result rc = _nfpCmdInIdOutU32(&g_nfpInterface, id, &tmp, 20);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpGetNpadId(HidControllerID id, u32 *out) {
    return _nfpCmdInIdOutU32(&g_nfpInterface, id, out, 21);
}

Result nfpAttachAvailabilityChangeEvent(Event *out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _fpuCmdGetEvent(&g_nfpInterface, out_event, true, 23);
}

Result nfcIsNfcEnabled(bool *out) {
    return _nfpCmdNoInOutBool(&g_nfcInterface, out, hosversionBefore(4,0,0) ? 3 : 403);
}
