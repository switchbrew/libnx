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
    ApmPerformanceMode_Invalid = -1,  ///< Invalid
    ApmPerformanceMode_Normal  = 0,   ///< Normal
    ApmPerformanceMode_Boost   = 1,   ///< Boost
} ApmPerformanceMode;

/// CpuBoostMode. With \ref appletSetCpuBoostMode, only values 0/1 are available. This allows using higher clock rates.
typedef enum {
    ApmCpuBoostMode_Normal   = 0,  ///< Default, boost-mode disabled.
    ApmCpuBoostMode_FastLoad = 1,  ///< Boost CPU. Additionally, throttle GPU to minimum. Use performance configurations 0x92220009 (Docked) and 0x9222000A (Handheld), or 0x9222000B and 0x9222000C.
    ApmCpuBoostMode_Type2    = 2,  ///< Conserve power. Only throttle GPU to minimum. Use performance configurations 0x9222000B and 0x9222000C.
} ApmCpuBoostMode;

/// Initialize apm. Used automatically by \ref appletInitialize with AppletType_Application.
Result apmInitialize(void);

/// Exit apm. Used automatically by \ref appletExit with AppletType_Application.
void apmExit(void);

/// Gets the Service object for the actual apm service session.
Service* apmGetServiceSession(void);

/// Gets the Service object for ISession.
Service* apmGetServiceSession_Session(void);

/**
 * @brief Gets the current ApmPerformanceMode.
 * @param[out] out_performanceMode ApmPerformanceMode
 */
Result apmGetPerformanceMode(ApmPerformanceMode* out_performanceMode);

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
