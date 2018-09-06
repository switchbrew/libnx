/**
 * @file psm.h
 * @brief PSM service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result psmInitialize(void);
void psmExit(void);

Result psmGetBatteryChargePercentage(u32 *out);
