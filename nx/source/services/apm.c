#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/apm.h"

static Service g_apmSrv;
static Service g_apmISession;

static Result _apmCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id);

NX_GENERATE_SERVICE_GUARD(apm);

Result _apmInitialize(void) {
    Result rc = smGetService(&g_apmSrv, "apm");

    // OpenSession.
    // Official sw doesn't open this until using commands which need it, when it wasn't already opened.
    if (R_SUCCEEDED(rc)) rc = _apmCmdGetSession(&g_apmSrv, &g_apmISession, 0);

    return rc;
}

void _apmCleanup(void) {
    serviceClose(&g_apmISession);
    serviceClose(&g_apmSrv);
}

Service* apmGetServiceSession(void) {
    return &g_apmSrv;    
}

Service* apmGetServiceSession_Session(void) {
    return &g_apmISession;
}

static Result _apmCmdGetSession(Service* srv, Service* srv_out, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );
}

Result apmSetPerformanceConfiguration(ApmPerformanceMode PerformanceMode, u32 PerformanceConfiguration) {
    const struct {
        u32 PerformanceMode;
        u32 PerformanceConfiguration;
    } in = { PerformanceMode, PerformanceConfiguration };

    return serviceDispatchIn(&g_apmISession, 0, in);
}

Result apmGetPerformanceConfiguration(ApmPerformanceMode PerformanceMode, u32 *PerformanceConfiguration) {
    u32 tmp=PerformanceMode;
    return serviceDispatchInOut(&g_apmISession, 1, tmp, *PerformanceConfiguration);
}

