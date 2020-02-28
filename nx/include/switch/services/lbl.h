/**
 * @file lbl.h
 * @brief LBL service IPC wrapper.
 * @author SciresM, exelix
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    LblBacklightSwitchStatus_Disabled  = 0,
    LblBacklightSwitchStatus_Enabled   = 1,
    LblBacklightSwitchStatus_Enabling  = 2,
    LblBacklightSwitchStatus_Disabling = 3,
} LblBacklightSwitchStatus;

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
Result lblGetBacklightSwitchStatus(LblBacklightSwitchStatus *out_value);

Result lblEnableDimming(void);
Result lblDisableDimming(void);
Result lblIsDimmingEnabled(bool *out_value);

Result lblEnableAutoBrightnessControl(void);
Result lblDisableAutoBrightnessControl(void);
Result lblIsAutoBrightnessControlEnabled(bool *out_value);

Result lblSetAmbientLightSensorValue(float value);

/**
 * @note Used internally by \ref appletGetAmbientLightSensorValue and \ref appletGetCurrentIlluminanceEx.
 */
Result lblGetAmbientLightSensorValue(bool *over_limit, float *lux);

/**
 * @note Only available on [3.0.0+].
 * @note Used internally by \ref appletIsIlluminanceAvailable.
 */
Result lblIsAmbientLightSensorAvailable(bool *out_value);

/**
 * @note Only available on [3.0.0+].
 */
Result lblSetCurrentBrightnessSettingForVrMode(float brightness);

/**
 * @note Only available on [3.0.0+].
 */
Result lblGetCurrentBrightnessSettingForVrMode(float *out_value);

/**
 * @note Only available on [3.0.0+].
 * @note Used internally by \ref appletSetVrModeEnabled.
 */
Result lblEnableVrMode(void);

/**
 * @note Only available on [3.0.0+].
 * @note Used internally by \ref appletSetVrModeEnabled.
 */
Result lblDisableVrMode(void);

/**
 * @note Only available on [3.0.0+].
 * @note Used internally by \ref appletIsVrModeEnabled.
 */
Result lblIsVrModeEnabled(bool *out_value);
