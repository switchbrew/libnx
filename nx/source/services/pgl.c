#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/pgl.h"
#include "runtime/hosversion.h"

static Service g_pglSrv;

NX_GENERATE_SERVICE_GUARD(pgl);

Result _pglInitialize(void) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return smGetService(&g_pglSrv, "pgl");
}

void _pglCleanup(void) {
    serviceClose(&g_pglSrv);
}

Service* pglGetServiceSession(void) {
    return &g_pglSrv;
}

static Result _pglCmdInU64(Service* srv, u32 cmd_id, u64 inval) {
    return serviceDispatchIn(srv, cmd_id, inval);
}

Result pglTerminateProcess(u64 pid) {
    return _pglCmdInU64(&g_pglSrv, 1, pid);
}
