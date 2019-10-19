#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/spsm.h"

static Service g_spsmSrv;

NX_GENERATE_SERVICE_GUARD(spsm);

Result _spsmInitialize(void) {
    return smGetService(&g_spsmSrv, "spsm");
}

void _spsmCleanup(void) {
    serviceClose(&g_spsmSrv);
}

Service* spsmGetServiceSession(void) {
    return &g_spsmSrv;
}

Result spsmShutdown(bool reboot) {
    const u8 in = reboot != 0;
    return serviceDispatchIn(&g_spsmSrv, 3, in);
}

Result spsmPutErrorState(void) {
    return serviceDispatch(&g_spsmSrv, 10);
}
