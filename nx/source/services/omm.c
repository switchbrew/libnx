#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/omm.h"
#include "runtime/hosversion.h"

static Service g_ommSrv;

NX_GENERATE_SERVICE_GUARD(omm);

Result _ommInitialize(void) {
    return smGetService(&g_ommSrv, "omm");
}

void _ommCleanup(void) {
    serviceClose(&g_ommSrv);
}

Service* ommGetServiceSession(void) {
    return &g_ommSrv;
}

Result ommGetDefaultDisplayResolution(s32* width, s32* height) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    struct {
        s32 width;
        s32 height;
    } out;

    Result rc = serviceDispatchOut(&g_ommSrv, 11, out);

    if (R_SUCCEEDED(rc)) {
        *width = out.width;
        *height = out.height;
    }

    return rc;
}

Result ommSetOperationModePolicy(ommOperationModePolicy value) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp = value;
    return serviceDispatchIn(&g_ommSrv, 10, tmp);
}