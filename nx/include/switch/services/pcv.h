/**
 * @file pcv.h
 * @brief PCV service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    PcvModule_Cpu = 0,
    PcvModule_Gpu = 1,
    PcvModule_Emc = 56,
} PcvModule;

Result pcvInitialize(void);
void pcvExit(void);

Result pcvGetClockRate(PcvModule module, u32 *out_hz);
Result pcvSetClockRate(PcvModule module, u32 hz);
Result pcvSetVoltageEnabled(bool state, u32 voltage);
Result pcvGetVoltageEnabled(bool *isEnabled, u32 voltage);

