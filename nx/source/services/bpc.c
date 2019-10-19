#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/bpc.h"
#include "runtime/hosversion.h"

static Service g_bpcSrv;

NX_GENERATE_SERVICE_GUARD(bpc);

Result _bpcInitialize(void) {
    return smGetService(&g_bpcSrv, hosversionAtLeast(2,0,0) ? "bpc" : "bpc:c");
}

void _bpcCleanup(void) {
    serviceClose(&g_bpcSrv);
}

Service* bpcGetServiceSession(void) {
    return &g_bpcSrv;
}

static Result _bpcCmdNoIO(Service *srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

Result bpcShutdownSystem(void) {
    return _bpcCmdNoIO(&g_bpcSrv, 0);
}

Result bpcRebootSystem(void) {
    return _bpcCmdNoIO(&g_bpcSrv, 1);
}

Result bpcGetSleepButtonState(BpcSleepButtonState *out) {
    u8 tmp = 0;
    Result rc = serviceDispatchOut(&g_bpcSrv, 6, tmp);

    if (R_SUCCEEDED(rc) && out) *out = tmp;

    return rc;
}
