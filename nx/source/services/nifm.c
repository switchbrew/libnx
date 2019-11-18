#include "service_guard.h"
#include "services/nifm.h"
#include "runtime/hosversion.h"

static NifmServiceType g_nifmServiceType;

static Service g_nifmSrv;
static Service g_nifmIGS;

static Result _nifmCreateGeneralService(Service* srv_out);
static Result _nifmCreateGeneralServiceOld(Service* srv_out);

NX_GENERATE_SERVICE_GUARD_PARAMS(nifm, (NifmServiceType service_type), (service_type));

Result _nifmInitialize(NifmServiceType service_type) {
    Result rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    g_nifmServiceType = service_type;
    switch (g_nifmServiceType) {
        case NifmServiceType_User:
            rc = smGetService(&g_nifmSrv, "nifm:u");
            break;
        case NifmServiceType_System:
            rc = smGetService(&g_nifmSrv, "nifm:s");
            break;
        case NifmServiceType_Admin:
            rc = smGetService(&g_nifmSrv, "nifm:a");
            break;
    }
    
    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_nifmSrv);

    if (R_SUCCEEDED(rc)) {
        if (hosversionAtLeast(3,0,0))
            rc = _nifmCreateGeneralService(&g_nifmIGS);
        else
            rc = _nifmCreateGeneralServiceOld(&g_nifmIGS);
    }

    return rc;
}

void _nifmCleanup(void) {
    serviceClose(&g_nifmIGS);
    serviceClose(&g_nifmSrv);
}

Service* nifmGetServiceSession_StaticService(void) {
    return &g_nifmSrv;
}

Service* nifmGetServiceSession_GeneralService(void) {
    return &g_nifmIGS;
}

static Result _nifmCmdNoIO(Service* srv, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id);
}

static Result _nifmCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _nifmCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nifmCmdNoInOutU8(Service* srv, u8 *out, u32 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _nifmCmdNoInOutBool(Service* srv, bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _nifmCmdNoInOutU8(srv, &tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _nifmCmdInU8NoOut(Service* srv, u8 inval, u64 cmd_id) {
    serviceAssumeDomain(srv);
    return serviceDispatchIn(srv, cmd_id, inval);
}

static Result _nifmCmdInBoolNoOut(Service* srv, bool inval, u32 cmd_id) {
    return _nifmCmdInU8NoOut(srv, inval!=0, cmd_id);
}

static Result _nifmCreateGeneralServiceOld(Service* srv_out) {
    return _nifmCmdGetSession(&g_nifmSrv, srv_out, 4);
}

static Result _nifmCreateGeneralService(Service* srv_out) {
    u64 reserved=0;
    serviceAssumeDomain(&g_nifmSrv);
    return serviceDispatchIn(&g_nifmSrv, 5, reserved,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result nifmGetCurrentIpAddress(u32* out) {
    return _nifmCmdNoInOutU32(&g_nifmIGS, out, 12);
}

Result nifmSetWirelessCommunicationEnabled(bool enable) {
    if (g_nifmServiceType < NifmServiceType_System)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return _nifmCmdInBoolNoOut(&g_nifmIGS, enable, 16);
}

Result nifmIsWirelessCommunicationEnabled(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 17);
}

Result nifmGetInternetConnectionStatus(NifmInternetConnectionType* connectionType, u32* wifiStrength, NifmInternetConnectionStatus* connectionStatus) {
    struct {
        u8 out1;
        u8 out2;
        u8 out3;
    } out;

    serviceAssumeDomain(&g_nifmIGS);
    Result rc = serviceDispatchOut(&g_nifmIGS, 18, out);
    if (R_SUCCEEDED(rc)) {
        if (connectionType) *connectionType = out.out1;
        if (wifiStrength) *wifiStrength = out.out2;
        if (connectionStatus) *connectionStatus = out.out3;
    }
    return rc;
}

Result nifmIsEthernetCommunicationEnabled(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 20);
}

Result nifmIsAnyForegroundRequestAccepted(bool* out) {
    return _nifmCmdNoInOutBool(&g_nifmIGS, out, 22);
}

Result nifmPutToSleep(void) {
    return _nifmCmdNoIO(&g_nifmIGS, 23);
}

Result nifmWakeUp(void) {
    return _nifmCmdNoIO(&g_nifmIGS, 24);
}
