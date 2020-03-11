#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/tc.h"

static Service g_tcSrv;

NX_GENERATE_SERVICE_GUARD(tc);

Result _tcInitialize(void) {
    return smGetService(&g_tcSrv, "tc");
}

void _tcCleanup(void) {
    serviceClose(&g_tcSrv);
}

Service* tcGetServiceSession(void) {
    return &g_tcSrv;
}

static Result _tcNoInNoOut(u32 cmd_id) {
    return serviceDispatch(&g_tcSrv, cmd_id);
}

Result tcEnableFanControl(void) {
    return _tcNoInNoOut(6);
}

Result tcDisableFanControl(void) {
    return _tcNoInNoOut(7);
}

Result tcIsFanControlEnabled(bool *status) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_tcSrv, 8, tmp);
    if (R_SUCCEEDED(rc) && status) *status = tmp & 1;
    return rc;
}

Result tcGetSkinTemperatureMilliC(s32 *skinTemp) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&g_tcSrv, 9, *skinTemp);
}
