/**
 * @file apm.h
 * @brief Performance management (apm) service IPC wrapper. This is used internally by applet with __nx_applet_PerformanceConfiguration, however if you prefer non-init/exit can be used manually. See also: https://switchbrew.org/wiki/PTM_services#apm:am
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// PerformanceMode
typedef enum {
    ApmPerformanceMode_Handheld = 0,   ///< Handheld
    ApmPerformanceMode_Docked   = 1,   ///< Docked
} ApmPerformanceMode;

/// CpuBoostMode. With \ref appletSetCpuBoostMode, only values 0/1 are available. This allows using higher clock rates.
typedef enum {
    ApmCpuBoostMode_Disabled = 0,  ///< Default, use normal PerformanceConfiguration.
    ApmCpuBoostMode_Type1    = 1,  ///< Use performance configurations 0x92220009 (Docked) and 0x9222000A (Handheld), or 0x9222000B and 0x9222000C. All of these use the normal GPU clock rate for Docked-mode. The latter pair uses the normal CPU clock rate, while the former pair uses the maximum TX1 CPU clock rate. Memory clock rate is the same as normal.
    ApmCpuBoostMode_Type2    = 2,  ///< Use performance configurations 0x9222000B and 0x9222000C.
} ApmCpuBoostMode;

/// Initialize apm. Used automatically by \ref appletInitialize.
Result apmInitialize(void);

/// Exit apm. Used automatically by \ref appletExit.
void apmExit(void);

/// Gets the Service object for the actual apm service session.
Service* apmGetServiceSession(void);

/// Gets the Service object for ISession.
Service* apmGetServiceSession_Session(void);

/**
 * @brief Sets the PerformanceConfiguration for the specified PerformanceMode.
 * @param[in] PerformanceMode \ref ApmPerformanceMode
 * @param[in] PerformanceConfiguration PerformanceConfiguration
 */
Result apmSetPerformanceConfiguration(ApmPerformanceMode PerformanceMode, u32 PerformanceConfiguration);

/**
 * @brief Gets the PerformanceConfiguration for the specified PerformanceMode.
 * @param[in] PerformanceMode \ref ApmPerformanceMode
 * @param[out] PerformanceConfiguration PerformanceConfiguration
 */
Result apmGetPerformanceConfiguration(ApmPerformanceMode PerformanceMode, u32 *PerformanceConfiguration);
