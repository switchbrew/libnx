#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/gpio.h"
#include "runtime/hosversion.h"

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

static Result _gpioCmdNoInNoOut(Service *srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

static Result _gpioCmdInU32NoOut(Service *srv, u32 value, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, value);
}

static Result _gpioCmdNoInOutU32(Service *srv, u32 *out_value, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out_value);
}

static Result _gpioCmdInBoolNoOut(Service *srv, bool value, u32 cmd_id) {
    const u8 in = value;
    return serviceDispatchIn(srv, cmd_id, in);
}

static Result _gpioCmdNoInOutBool(Service *srv, bool *out_value, u32 cmd_id) {
    u8 outval = 0;
    Result rc = serviceDispatchOut(srv, cmd_id, outval);
    if (R_SUCCEEDED(rc)) {
        if (out_value) *out_value = outval & 1;
    }
    return rc;
}

static Result _gpioCmdInU32OutBool(Service *srv, bool *out_value, u32 inval, u32 cmd_id) {
    u8 outval = 0;
    Result rc = serviceDispatchInOut(srv, cmd_id, inval, outval);
    if (R_SUCCEEDED(rc)) {
        if (out_value) *out_value = outval & 1;
    }
    return rc;
}

Result gpioOpenSession(GpioPadSession *out, GpioPadName name) {
    _Static_assert(sizeof(name) == sizeof(u32), "GpioPadName size");
    return serviceDispatchIn(&g_gpioSrv, 1, name,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result gpioOpenSession2(GpioPadSession *out, u32 device_code, u32 access_mode) {
    if (hosversionBefore(7,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    const struct {
        u32 device_code;
        u32 access_mode;
    } in = { device_code, access_mode };

    return serviceDispatchIn(&g_gpioSrv, 7, in,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Result gpioIsWakeEventActive(bool *out, GpioPadName name) {
    if (hosversionAtLeast(7,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    _Static_assert(sizeof(name) == sizeof(u32), "GpioPadName size");
    return _gpioCmdInU32OutBool(&g_gpioSrv, out, name, 3);
}

Result gpioIsWakeEventActive2(bool *out, u32 device_code) {
    if (hosversionBefore(7,0,0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    return _gpioCmdInU32OutBool(&g_gpioSrv, out, device_code, 8);
}

Result gpioPadSetDirection(GpioPadSession *p, GpioDirection dir) {
    return _gpioCmdInU32NoOut(&p->s, dir, 0);
}

Result gpioPadGetDirection(GpioPadSession *p, GpioDirection *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioDirection size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 1);
}

Result gpioPadSetInterruptMode(GpioPadSession *p, GpioInterruptMode mode) {
    return _gpioCmdInU32NoOut(&p->s, mode, 2);
}

Result gpioPadGetInterruptMode(GpioPadSession *p, GpioInterruptMode *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioInterruptMode size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 3);
}

Result gpioPadSetInterruptEnable(GpioPadSession *p, bool en) {
    return _gpioCmdInBoolNoOut(&p->s, en, 4);
}

Result gpioPadGetInterruptEnable(GpioPadSession *p, bool *out) {
    return _gpioCmdNoInOutBool(&p->s, out, 5);
}

Result gpioPadGetInterruptStatus(GpioPadSession *p, GpioInterruptStatus *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioInterruptStatus size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 6);
}

Result gpioPadClearInterruptStatus(GpioPadSession *p) {
    return _gpioCmdNoInNoOut(&p->s, 7);
}


Result gpioPadSetValue(GpioPadSession *p, GpioValue val) {
    return _gpioCmdInU32NoOut(&p->s, val, 8);
}

Result gpioPadGetValue(GpioPadSession *p, GpioValue *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "GpioValue size");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 9);
}

Result gpioPadBindInterrupt(GpioPadSession *p, Event *out) {
    Handle evt_handle;
    Result rc = serviceDispatch(&p->s, 10,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(out, evt_handle, false);
    }

    return rc;
}

Result gpioPadUnbindInterrupt(GpioPadSession *p) {
    return _gpioCmdNoInNoOut(&p->s, 11);
}

Result gpioPadSetDebounceEnabled(GpioPadSession *p, bool en) {
    return _gpioCmdInBoolNoOut(&p->s, en, 12);
}

Result gpioPadGetDebounceEnabled(GpioPadSession *p, bool *out) {
    return _gpioCmdNoInOutBool(&p->s, out, 13);
}

Result gpioPadSetDebounceTime(GpioPadSession *p, s32 ms) {
    return _gpioCmdInU32NoOut(&p->s, (u32)ms, 14);
}

Result gpioPadGetDebounceTime(GpioPadSession *p, s32 *out) {
    _Static_assert(sizeof(*out) == sizeof(u32), "");
    return _gpioCmdNoInOutU32(&p->s, (u32 *)out, 15);
}

void gpioPadClose(GpioPadSession *p) {
    serviceClose(&p->s);
}
