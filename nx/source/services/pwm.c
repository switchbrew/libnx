#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/pwm.h"
#include "runtime/hosversion.h"

static Service g_pwmSrv;

NX_GENERATE_SERVICE_GUARD(pwm);

Result _pwmInitialize(void) {
    return smGetService(&g_pwmSrv, "pwm");
}

void _pwmCleanup(void) {
    serviceClose(&g_pwmSrv);
}

Service* pwmGetServiceSession(void) {
    return &g_pwmSrv;
}

Result pwmOpenSession2(PwmChannelSession *out, PwmChannelDeviceCode device_code) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u32 tmp = device_code;

    return serviceDispatchIn(&g_pwmSrv, 2, tmp,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result pwmChannelSessionGetDutyCycle(PwmChannelSession *c, double* out) {
    if (hosversionBefore(6,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (!serviceIsActive(&c->s))
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    return serviceDispatchOut(&c->s, 7, *out);
}

void pwmChannelSessionClose(PwmChannelSession *c) {
    serviceClose(&c->s);
}
