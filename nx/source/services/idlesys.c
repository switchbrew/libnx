#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/idlesys.h"

static Service g_idlesysSrv;

NX_GENERATE_SERVICE_GUARD(idlesys);

Result _idlesysInitialize(void) {
    return smGetService(&g_idlesysSrv, "idle:sys");
}

void _idlesysCleanup(void) {
    serviceClose(&g_idlesysSrv);
}

Service* idlesysGetServiceSession(void) {
    return &g_idlesysSrv;
}

Result idlesysReportUserIsActive(void) {
    return serviceDispatch(&g_idlesysSrv, 5);
}
