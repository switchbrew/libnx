#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/lbl.h"

static Service g_lblSrv;

NX_GENERATE_SERVICE_GUARD(lbl);

Result _lblInitialize(void) {
    return smGetService(&g_lblSrv, "lbl");
}

void _lblCleanup(void) {
    serviceClose(&g_lblSrv);
}

Service* lblGetServiceSession(void) {
    return &g_lblSrv;
}

static Result _lblCmdNoIO(u32 cmd_id) {
    return serviceDispatch(&g_lblSrv, cmd_id);
}

static Result _lblCmdInU64NoOut(u64 inval, u32 cmd_id) {
    return serviceDispatchIn(&g_lblSrv, cmd_id, inval);
}

static Result _lblCmdNoInOutU8(u8 *out, u32 cmd_id) {
    return serviceDispatchOut(&g_lblSrv, cmd_id, *out);
}

static Result _lblCmdNoInOutBool(bool *out, u32 cmd_id) {
    u8 tmpout=0;
    Result rc = _lblCmdNoInOutU8(&tmpout, cmd_id);
    if (R_SUCCEEDED(rc) && out) *out = tmpout & 1;
    return rc;
}

Result lblSwitchBacklightOn(u64 fade_time) {
    return _lblCmdInU64NoOut(fade_time, 6);
}

Result lblSwitchBacklightOff(u64 fade_time) {
    return _lblCmdInU64NoOut(fade_time, 7);
}

Result lblSetCurrentBrightnessSetting(float brightness) {
    return serviceDispatchIn(&g_lblSrv, 2, brightness);
}

Result lblGetCurrentBrightnessSetting(float *out_value) {
    return serviceDispatchOut(&g_lblSrv, 3, *out_value);
}

Result lblEnableAutoBrightnessControl(void) {
    return _lblCmdNoIO(12);
}

Result lblDisableAutoBrightnessControl(void) {
    return _lblCmdNoIO(13);    
}

Result lblIsAutoBrightnessControlEnabled(bool *out_value){
    return _lblCmdNoInOutBool(out_value, 14);
}
