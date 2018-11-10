/**
 * @file lbl.h
 * @brief LBL service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result lblInitialize(void);
void lblExit(void);

Result lblSwitchBacklightOn(u64 fade_time);
Result lblSwitchBacklightOff(u64 fade_time);
