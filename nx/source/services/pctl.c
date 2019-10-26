#include "service_guard.h"
#include "services/pctl.h"
#include "runtime/hosversion.h"

static Service g_pctlSrv;
static Service g_pctlSession;

static Result _pctlCreateService(Service* srv_out, u32 cmd_id);
static Result _pctlCmdNoIO(u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(pctl);

Result _pctlInitialize(void) {
    Result rc=0;
    bool sysverflag = hosversionBefore(4,0,0);

    rc = smGetService(&g_pctlSrv, "pctl:a");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl:s");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl:r");
    if (R_FAILED(rc)) rc = smGetService(&g_pctlSrv, "pctl");

    if (R_SUCCEEDED(rc)) rc = serviceConvertToDomain(&g_pctlSrv);

    if (R_SUCCEEDED(rc)) rc = _pctlCreateService(&g_pctlSession, sysverflag ? 0 : 1);

    if (R_SUCCEEDED(rc) && !sysverflag) rc = _pctlCmdNoIO(1);

    return rc;
}

void _pctlCleanup(void) {
    serviceClose(&g_pctlSession);
    serviceClose(&g_pctlSrv);
}

Service* pctlGetServiceSession(void) {
    return &g_pctlSrv;
}

Service* pctlGetServiceSession_Service(void) {
    return &g_pctlSession;
}

static Result _pctlCreateService(Service* srv_out, u32 cmd_id) {
    u64 pid_reserved=0;
    serviceAssumeDomain(&g_pctlSrv);
    return serviceDispatchIn(&g_pctlSrv, cmd_id, pid_reserved,
        .in_send_pid = true,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

static Result _pctlCmdNoIO(u32 cmd_id) {
    serviceAssumeDomain(&g_pctlSession);
    return serviceDispatch(&g_pctlSession, cmd_id);
}

static Result _pctlCmdNoInOutU8(u8 *out, u32 cmd_id) {
    serviceAssumeDomain(&g_pctlSession);
    return serviceDispatchOut(&g_pctlSession, cmd_id, *out);
}

static Result _pctlCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmp=0;
    Result rc = _pctlCmdNoInOutU8(&tmp, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result pctlConfirmStereoVisionPermission(void) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pctlCmdNoIO(1013);
}

Result pctlIsRestrictionEnabled(bool *flag) {
    return _pctlCmdNoInOutBool(flag, 1031);
}

Result pctlResetConfirmedStereoVisionPermission(void) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pctlCmdNoIO(1064);
}

Result pctlIsStereoVisionPermitted(bool *flag) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return _pctlCmdNoInOutBool(flag, 1065);
}

