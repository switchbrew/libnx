/**
 * @file lbl.h
 * @brief LBL service IPC wrapper.
 * @author SciresM, exelix
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize lbl.
Result lblInitialize(void);

/// Exit lbl.
void lblExit(void);

/// Gets the Service object for the actual lbl service session.
Service* lblGetServiceSession(void);

Result lblSaveCurrentSetting(void);
Result lblLoadCurrentSetting(void);

/**
 * @note The brightness goes from 0 to 1.0.
 */
Result lblSetCurrentBrightnessSetting(float brightness);
Result lblGetCurrentBrightnessSetting(float *out_value);

Result lblApplyCurrentBrightnessSettingToBacklight(void);
Result lblGetBrightnessSettingAppliedToBacklight(float *out_value);

Result lblSwitchBacklightOn(u64 fade_time);
Result lblSwitchBacklightOff(u64 fade_time);
Result lblGetBacklightSwitchStatus(bool *out_value);

Result lblEnableDimming(void);
Result lblDisableDimming(void);
Result lblIsDimmingEnabled(bool *out_value);

Result lblEnableAutoBrightnessControl(void);
Result lblDisableAutoBrightnessControl(void);
Result lblIsAutoBrightnessControlEnabled(bool *out_value);

Result lblSetAmbientLightSensorValue(float value);
Result lblGetAmbientLightSensorValue(bool *over_limit, float *lux);

/* Only available on [3.0.0+] */
Result lblIsAmbientLightSensorAvailable(bool *out_value);
Result lblSetCurrentBrightnessSettingForVrMode(float brightness);
Result lblGetCurrentBrightnessSettingForVrMode(float *out_value);
Result lblEnableVrMode(void);
Result lblDisableVrMode(void);
Result lblIsVrModeEnabled(bool *out_value);
