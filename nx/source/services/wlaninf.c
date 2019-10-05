#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/wlaninf.h"

static Service g_wlaninfSrv;

NX_GENERATE_SERVICE_GUARD(wlaninf);

Result _wlaninfInitialize(void) {
    return smGetService(&g_wlaninfSrv, "wlan:inf");
}

void _wlaninfCleanup(void) {
    serviceClose(&g_wlaninfSrv);
}

Service* wlaninfGetServiceSession(void) {
    return &g_wlaninfSrv;
}

static Result _wlaninfCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

Result wlaninfGetState(WlanInfState* out) {
    u32 tmp=0;
    Result rc = _wlaninfCmdNoInOutU32(&g_wlaninfSrv, &tmp, 10);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result wlaninfGetRSSI(s32* out) {
    return _wlaninfCmdNoInOutU32(&g_wlaninfSrv, (u32*)out, 12);
}
