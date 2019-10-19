#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/gpio.h"

static Service g_gpioSrv;

NX_GENERATE_SERVICE_GUARD(gpio);

Result _gpioInitialize(void) {
    return smGetService(&g_gpioSrv, "gpio");
}

void _gpioCleanup(void) {
    serviceClose(&g_gpioSrv);
}

Service* gpioGetServiceSession(void) {
    return &g_gpioSrv;
}

static Result _gpioCmdInU32NoOut(Service *srv, u32 value, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, value);
}

static Result _gpioCmdNoInOutU32(Service *srv, u32 *out_value, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out_value);
}

Result gpioOpenSession(GpioPadSession *out, GpioPadName name) {
    _Static_assert(sizeof(name) == sizeof(u32), "GpioPadName size");
    return serviceDispatchIn(&g_gpioSrv, 1, name,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result gpioPadSetDirection(GpioPadSession *p, GpioDirection dir) {
    return _gpioCmdInU32NoOut(&p->s, dir, 0);
}

Result gpioPadGetDirection(GpioPadSession *p, GpioDirection *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioDirection size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 1);
}

Result gpioPadSetValue(GpioPadSession *p, GpioValue val) {
    return _gpioCmdInU32NoOut(&p->s, val, 8);
}

Result gpioPadGetValue(GpioPadSession *p, GpioValue *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioValue size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 9);
}

void gpioPadClose(GpioPadSession *p) {
    serviceClose(&p->s);
}
