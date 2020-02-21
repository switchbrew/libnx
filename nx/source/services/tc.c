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
    struct {
        u8 status;
    } out;

    Result rc = serviceDispatchOut(&g_tcSrv, 8, out);
    if (R_SUCCEEDED(rc)) *status = out.status;
    return rc;
}

Result tcGetSkinTemperatureMilliC(s32 *skinTemp) {
    if (hosversionBefore(5,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        s32 skinTemp;
    } out;

    Result rc = serviceDispatchOut(&g_tcSrv, 9, out);
    if (R_SUCCEEDED(rc)) *skinTemp = out.skinTemp;
    return rc;
}
