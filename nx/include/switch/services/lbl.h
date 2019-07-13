/**
 * @file lbl.h
 * @brief LBL service IPC wrapper.
 * @author SciresM, exelix
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/sm.h"

Result lblInitialize(void);
void lblExit(void);
Service* lblGetServiceSession(void);

Result lblSwitchBacklightOn(u64 fade_time);
Result lblSwitchBacklightOff(u64 fade_time);

/**
 * @note The brightness goes from 0 to 1.0.
 */
Result lblSetCurrentBrightnessSetting(float brightness);
Result lblGetCurrentBrightnessSetting(float *out_value);

Result lblEnableAutoBrightnessControl(void);
Result lblDisableAutoBrightnessControl(void);
Result lblIsAutoBrightnessControlEnabled(bool *out_value);
