#include "service_guard.h"
#include <string.h>
#include "runtime/hosversion.h"
#include "services/hid.h"
#include "services/applet.h"
#include "services/nfc.h"

static Service g_nfpuSrv;
static Service g_nfcuSrv;
static Service g_nfpuInterface;
static Service g_nfcuInterface;

// This is the data passed by every application this was tested with
static const NfpuInitConfig g_nfpuDefaultInitConfig = {
    .unk1 = 0x00000001000a0003,
    .reserved1 = {0},
    .unk2 = 0x0000000300040003,
    .reserved2 = {0},
};

static Result _nfpuCreateInterface(Service* srv, Service* srv_out);
static Result _nfpuInterfaceInitialize(Service* srv, u64 aruid, const NfpuInitConfig *config, u32 cmd_id);
static Result _nfpuInterfaceFinalize(Service* srv, u32 cmd_id);

static Result _nfpuCmdNoIO(Service* srv, u32 cmd_id);
static Result _nfpuCmdInIdNoOut(Service* srv, HidControllerID id, u32 cmd_id);
static Result _nfpuCmdInIdOutEvent(Service* srv, HidControllerID id, Event *out_event, u32 cmd_id);
static Result _nfpuCmdInIdOutBuffer(Service* srv, HidControllerID id, void* buf, size_t buf_size, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD_PARAMS(nfpu, (const NfpuInitConfig *config), (config));

Result _nfpuInitialize(const NfpuInitConfig *config) {
    if (config == NULL)
        config = &g_nfpuDefaultInitConfig;

    // If this fails (for example because we're a sysmodule) aruid stays zero
    u64 aruid = 0;
    appletGetAppletResourceUserId(&aruid);

    // nfp:user
    Result rc = smGetService(&g_nfpuSrv, "nfp:user");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfpuSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpuCreateInterface(&g_nfpuSrv, &g_nfpuInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpuInterfaceInitialize(&g_nfpuInterface, aruid, config, 0);

    // nfc:user
    if (R_SUCCEEDED(rc))
        rc = smGetService(&g_nfcuSrv, "nfc:user");

    if (R_SUCCEEDED(rc))
        rc = serviceConvertToDomain(&g_nfcuSrv);

    if (R_SUCCEEDED(rc))
        rc = _nfpuCreateInterface(&g_nfcuSrv, &g_nfcuInterface);

    if (R_SUCCEEDED(rc))
        rc = _nfpuInterfaceInitialize(&g_nfcuInterface, aruid, config, hosversionBefore(4,0,0) ? 0 : 400);

    return rc;
}

void _nfpuCleanup(void) {
    _nfpuInterfaceFinalize(&g_nfpuInterface, 1);
    _nfpuInterfaceFinalize(&g_nfcuInterface, hosversionBefore(4,0,0) ? 1 : 401);
    serviceClose(&g_nfpuInterface);
    serviceClose(&g_nfcuInterface);
    serviceClose(&g_nfpuSrv);
    serviceClose(&g_nfcuSrv);
}

Service* nfpuGetServiceSession(void) {
    return &g_nfpuSrv;
}

Service* nfpuGetServiceSession_Interface(void) {
    return &g_nfpuInterface;
}

Service* nfcuGetServiceSession(void) {
    return &g_nfpuSrv;
}

Service* nfcuGetServiceSession_Interface(void) {
    return &g_nfpuInterface;
}

static Result _nfpuCreateInterface(Service* srv, Service* srv_out) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, 0,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _nfpuCmdGetHandle(Service* srv, Handle* handle_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
}

static Result _fpuCmdGetEvent(Service* srv, Event* out_event, bool autoclear, u32 cmd_id) {
    Handle tmp_handle = INVALID_HANDLE;
    Result rc = 0;

    rc = _nfpuCmdGetHandle(srv, &tmp_handle, cmd_id);
    if (R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

static Result _nfpuCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _nfpuCmdInIdNoOut(Service* srv, HidControllerID id, u32 cmd_id) {
    serviceAssumeDomain(srv);
    u64 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchIn(srv, cmd_id, tmp);
}

static Result _nfpuCmdInIdOutU32(Service* srv, HidControllerID id, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    u64 tmp = hidControllerIDToOfficial(id);
    return serviceDispatchInOut(srv, cmd_id, tmp, *out);
}

static Result _nfpuCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfpuCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nfpuCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _nfpuCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp!=0;
    return rc;
}

static Result _nfpuCmdInIdOutEvent(Service* srv, HidControllerID id, Event *out_event, u32 cmd_id) {
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

static Result _nfpuCmdInIdOutBuffer(Service* srv, HidControllerID id, void* buf, size_t buf_size, u32 cmd_id) {
    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, tmp,
        .buffer_attrs = { SfBufferAttr_FixedSize | SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

static Result _nfpuInterfaceInitialize(Service* srv, u64 aruid, const NfpuInitConfig *config, u32 cmd_id) {
    const struct {
        u64 aruid;
        u64 zero;
    } in = { aruid, 0 };

    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { config, sizeof(NfpuInitConfig) } },
        .in_send_pid = true,
    );
}

static Result _nfpuInterfaceFinalize(Service* srv, u32 cmd_id) {
    return _nfpuCmdNoIO(srv, cmd_id);
}

Result nfpuListDevices(s32 *count, HidControllerID *out, size_t num_elements) {
    // This is the maximum number of controllers that can be connected to a console at a time
    // Incidentally, this is the biggest value official software (SSBU) was observed using
    size_t max_controllers = 9;
    if (num_elements > max_controllers)
        num_elements = max_controllers;

    u64 buf[max_controllers];
    memset(buf, 0, sizeof(buf));

    serviceAssumeDomain(&g_nfpuInterface);
    Result rc = serviceDispatchOut(&g_nfpuInterface, 2, *count,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { buf, max_controllers*sizeof(u64) } },
    );
    if (R_SUCCEEDED(rc) && out) {
        for (size_t i=0; i<num_elements; i++)
            out[i] = hidControllerIDFromOfficial(buf[i]);
    }
    return rc;
}

Result nfpuStartDetection(HidControllerID id) {
    return _nfpuCmdInIdNoOut(&g_nfpuInterface, id, 3);
}

Result nfpuStopDetection(HidControllerID id) {
    return _nfpuCmdInIdNoOut(&g_nfpuInterface, id, 4);
}

Result nfpuMount(HidControllerID id, NfpuDeviceType device_type, NfpuMountTarget mount_target) {
    const struct {
        u64 id;
        u32 device_type;
        u32 mount_target;
    } in = { hidControllerIDToOfficial(id), device_type, mount_target };

    serviceAssumeDomain(&g_nfpuInterface);
    return serviceDispatchIn(&g_nfpuInterface, 5, in);
}

Result nfpuUnmount(HidControllerID id) {
    return _nfpuCmdInIdNoOut(&g_nfpuInterface, id, 6);
}

Result nfpuOpenApplicationArea(HidControllerID id, u32 app_id, u32 *npad_id) {
    const struct {
        u64 id;
        u32 app_id;
    } in = { hidControllerIDToOfficial(id), app_id };

    serviceAssumeDomain(&g_nfpuInterface);
    return serviceDispatchInOut(&g_nfpuInterface, 7, in, *npad_id);
}

Result nfpuGetApplicationArea(HidControllerID id, void* buf, size_t buf_size) {
    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(&g_nfpuInterface);
    return serviceDispatchIn(&g_nfpuInterface, 8, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpuSetApplicationArea(HidControllerID id, const void* buf, size_t buf_size) {
    u64 tmp = hidControllerIDToOfficial(id);
    serviceAssumeDomain(&g_nfpuInterface);
    return serviceDispatchIn(&g_nfpuInterface, 9, tmp,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpuFlush(HidControllerID id) {
    return _nfpuCmdInIdNoOut(&g_nfpuInterface, id, 10);
}

Result nfpuRestore(HidControllerID id) {
    return _nfpuCmdInIdNoOut(&g_nfpuInterface, id, 11);
}

Result nfpuCreateApplicationArea(HidControllerID id, u32 app_id, const void* buf, size_t buf_size) {
    const struct {
        u64 id;
        u32 app_id;
    } PACKED in = { hidControllerIDToOfficial(id), app_id };

    serviceAssumeDomain(&g_nfpuInterface);
    return serviceDispatchIn(&g_nfpuInterface, 12, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
        .buffers = { { buf, buf_size } },
    );
}

Result nfpuGetTagInfo(HidControllerID id, NfpuTagInfo *out) {
    return _nfpuCmdInIdOutBuffer(&g_nfpuInterface, id, out, sizeof(NfpuTagInfo), 13);
}

Result nfpuGetRegisterInfo(HidControllerID id, NfpuRegisterInfo *out) {
    return _nfpuCmdInIdOutBuffer(&g_nfpuInterface, id, out, sizeof(NfpuRegisterInfo), 14);
}

Result nfpuGetCommonInfo(HidControllerID id, NfpuCommonInfo *out) {
    return _nfpuCmdInIdOutBuffer(&g_nfpuInterface, id, out, sizeof(NfpuCommonInfo), 15);
}

Result nfpuGetModelInfo(HidControllerID id, NfpuModelInfo *out) {
    return _nfpuCmdInIdOutBuffer(&g_nfpuInterface, id, out, sizeof(NfpuModelInfo), 16);
}

Result nfpuAttachActivateEvent(HidControllerID id, Event *out_event) {
    return _nfpuCmdInIdOutEvent(&g_nfpuInterface, id, out_event, 17);
}

Result nfpuAttachDeactivateEvent(HidControllerID id, Event *out_event) {
    return _nfpuCmdInIdOutEvent(&g_nfpuInterface, id, out_event, 18);
}

Result nfpuGetState(NfpuState *out) {
    u32 tmp=0;
    Result rc = _nfpuCmdNoInOutU32(&g_nfpuInterface, &tmp, 19);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpuGetDeviceState(HidControllerID id, NfpuDeviceState *out) {
    u32 tmp=0;
    Result rc = _nfpuCmdInIdOutU32(&g_nfpuInterface, id, &tmp, 20);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result nfpuGetNpadId(HidControllerID id, u32 *out) {
    return _nfpuCmdInIdOutU32(&g_nfpuInterface, id, out, 21);
}

Result nfpuAttachAvailabilityChangeEvent(Event *out_event) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _fpuCmdGetEvent(&g_nfpuInterface, out_event, true, 23);
}

Result nfpuIsNfcEnabled(bool *out) {
    return _nfpuCmdNoInOutBool(&g_nfcuInterface, out, hosversionBefore(4,0,0) ? 3 : 403);
}
