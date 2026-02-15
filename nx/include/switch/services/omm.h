/**
 * @file omm.h
 * @author MasaGratoR
 * @copyright libnx Authors
 */

#pragma once

#include "../types.h"
#include "../sf/service.h"

///OperationModePolicy
typedef enum {
    ommOperationModePolicy_Auto       = 0,
    ommOperationModePolicy_Handheld   = 1,
    ommOperationModePolicy_Console    = 2
} ommOperationModePolicy;

/// Initialize pwm.
Result ommInitialize(void);

/// Exit omm.
void ommExit(void);

/// Gets the Service for omm.
Service* ommGetServiceSession(void);

/// Returns a DefaultDisplayResolution. Only available on [3.0.0+].
Result ommGetDefaultDisplayResolution(s32* width, s32* height);

/// Takes an ommOperationModePolicy. Only available on [3.0.0+].
Result ommSetOperationModePolicy(ommOperationModePolicy value);