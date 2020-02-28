#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "service_guard.h"
#include "services/lbl.h"
#include "runtime/hosversion.h"

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

static Result _lblCmdNoInOutFloat(float *out, u32 cmd_id) {
    return serviceDispatchOut(&g_lblSrv, cmd_id, *out);
}

static Result _lblCmdInFloatNoOut(float in, u32 cmd_id) {
    return serviceDispatchIn(&g_lblSrv, cmd_id, in);
}

Result lblSaveCurrentSetting(void) {
    return _lblCmdNoIO(0);
}

Result lblLoadCurrentSetting(void) {
    return _lblCmdNoIO(1);
}

Result lblSetCurrentBrightnessSetting(float brightness) {
    return _lblCmdInFloatNoOut(brightness, 2);
}

Result lblGetCurrentBrightnessSetting(float *out_value) {
    return _lblCmdNoInOutFloat(out_value, 3);
}

Result lblApplyCurrentBrightnessSettingToBacklight(void) {
    return _lblCmdNoIO(4);
}

Result lblGetBrightnessSettingAppliedToBacklight(float *out_value) {
    return _lblCmdNoInOutFloat(out_value, 5);
}

Result lblSwitchBacklightOn(u64 fade_time) {
    return _lblCmdInU64NoOut(fade_time, 6);
}

Result lblSwitchBacklightOff(u64 fade_time) {
    return _lblCmdInU64NoOut(fade_time, 7);
}

Result lblGetBacklightSwitchStatus(LblBacklightSwitchStatus *out_value) {
    return serviceDispatchOut(&g_lblSrv, 8, *out_value);
}

Result lblEnableDimming(void) {
    return _lblCmdNoIO(9);
}

Result lblDisableDimming(void) {
    return _lblCmdNoIO(10);
}

Result lblIsDimmingEnabled(bool *out_value) {
    return _lblCmdNoInOutBool(out_value, 11);
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

Result lblSetAmbientLightSensorValue(float value) {
    return _lblCmdInFloatNoOut(value, 15);
}

Result lblGetAmbientLightSensorValue(bool *over_limit, float *lux) {
    struct {
        u32 over_limit;
        float lux;
    } out;

    Result rc = serviceDispatchOut(&g_lblSrv, 16, out);
    if (R_SUCCEEDED(rc)) {
        if (over_limit) *over_limit = out.over_limit & 1;
        if (lux) *lux = out.lux;
    }
    return rc;
}

Result lblIsAmbientLightSensorAvailable(bool *out_value) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdNoInOutBool(out_value, 23);
}
Result lblSetCurrentBrightnessSettingForVrMode(float brightness) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdInFloatNoOut(brightness, 24);
}
Result lblGetCurrentBrightnessSettingForVrMode(float *out_value) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdNoInOutFloat(out_value, 25);
}

Result lblEnableVrMode(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdNoIO(26);
}

Result lblDisableVrMode(void) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdNoIO(27);
}

Result lblIsVrModeEnabled(bool *out_value) {
    if (hosversionBefore(3,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    return _lblCmdNoInOutBool(out_value, 28);
}
