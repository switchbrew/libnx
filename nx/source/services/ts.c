#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/ts.h"

static Service g_tsSrv;

NX_GENERATE_SERVICE_GUARD(ts);

Result _tsInitialize(void) {
    return smGetService(&g_tsSrv, "ts");
}

void _tsCleanup(void) {
    serviceClose(&g_tsSrv);
}

Service* tsGetServiceSession(void) {
    return &g_tsSrv;
}

static Result _tsCmdInU8Out32(u8 inval, u32 *out, u64 cmd_id) {
    return serviceDispatchInOut(&g_tsSrv, cmd_id, inval, *out);
}

Result tsGetTemperatureRange(TsLocation location, s32 *min_temperature, s32 *max_temperature) {
    if (hosversionAtLeast(17,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    u8 tmp_location = location;

    struct {
        s32 min_temperature;
        s32 max_temperature;
    } out;

    Result rc = serviceDispatchInOut(&g_tsSrv, 0, tmp_location, out);
    if (R_SUCCEEDED(rc) && min_temperature) *min_temperature = out.min_temperature;
    if (R_SUCCEEDED(rc) && max_temperature) *max_temperature = out.max_temperature;
    return rc;
}

Result tsGetTemperature(TsLocation location, s32 *temperature) {
    if (hosversionAtLeast(17,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _tsCmdInU8Out32(location, (u32*)temperature, 1);
}

Result tsGetTemperatureMilliC(TsLocation location, s32 *temperature) {
    if (hosversionAtLeast(14,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _tsCmdInU8Out32(location, (u32*)temperature, 3);
}

Result tsOpenSession(TsSession *s, u32 device_code) {
    if (hosversionBefore(8,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchIn(&g_tsSrv, 4, device_code,
        .out_num_objects = 1,
        .out_objects = &s->s,
    );
}

Result tsSessionGetTemperature(TsSession *s, float *temperature) {
    if (hosversionBefore(10,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchOut(&s->s, 4, *temperature);
}

void tsSessionClose(TsSession *s) {
    serviceClose(&s->s);
}
