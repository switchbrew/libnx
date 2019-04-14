/**
 * @file apm.h
 * @brief Performance management (apm) service IPC wrapper. This is used internally by applet with __nx_applet_PerformanceConfiguration, however if you prefer non-init/exit can be used manually.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// CpuBoostMode. With \ref appletSetCpuBoostMode, only values 0/1 are available. This allows using higher clock rates.
typedef enum {
    ApmCpuBoostMode_Disabled = 0,  ///< Default, use normal PerformanceConfiguration.
    ApmCpuBoostMode_Type1    = 1,  ///< Use performance configurations 0x92220009 (Docked) and 0x9222000A (Handheld), or 0x9222000B and 0x9222000C. All of these use the normal GPU clock rate for Docked-mode. The latter pair uses the normal CPU clock rate, while the former pair uses the maximum TX1 CPU clock rate. Memory clock rate is the same as normal.
    ApmCpuBoostMode_Type2    = 2,  ///< Use performance configurations 0x9222000B and 0x9222000C.
} ApmCpuBoostMode;

Result apmInitialize(void);
void apmExit(void);

Result apmSetPerformanceConfiguration(u32 PerformanceMode, u32 PerformanceConfiguration);
Result apmGetPerformanceConfiguration(u32 PerformanceMode, u32 *PerformanceConfiguration);
