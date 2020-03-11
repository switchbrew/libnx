#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/fan.h"
#include "runtime/hosversion.h"

static Service g_fanSrv;

NX_GENERATE_SERVICE_GUARD(fan);

Result _fanInitialize(void) {
    return smGetService(&g_fanSrv, "fan");
}

void _fanCleanup(void) {
    serviceClose(&g_fanSrv);
}

Result fanOpenController(FanController *out, u32 device_code) {
    return serviceDispatchIn(&g_fanSrv, 0, device_code,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

Service* fanGetServiceSession(void) {
    return &g_fanSrv;
}

void fanControllerClose(FanController *controller) {
    serviceClose(&controller->s);
}

Result fanControllerSetRotationSpeedLevel(FanController *controller, float level) {
    return serviceDispatchIn(&controller->s, 0, level);
}

Result fanControllerGetRotationSpeedLevel(FanController *controller, float *level) {
    return serviceDispatchOut(&controller->s, 2, *level);
}
