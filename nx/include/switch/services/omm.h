/**
 * @file omm.h
 * @author MasaGratoR
 * @copyright libnx Authors
 */

#pragma once

#include "../types.h"
#include "../sf/service.h"

///OperationMode
typedef enum {
    OmmOperationMode_Handheld   = 0,
    OmmOperationMode_Console    = 1
} OmmOperationMode;

///OperationModePolicy
typedef enum {
    OmmOperationModePolicy_Auto       = 0,
    OmmOperationModePolicy_Handheld   = 1,
    OmmOperationModePolicy_Console    = 2
} OmmOperationModePolicy;

/// Initialize omm.
Result ommInitialize(void);

/// Exit omm.
void ommExit(void);

/// Gets the Service for omm.
Service* ommGetServiceSession(void);

/// Returns a DefaultDisplayResolution. Only available on [3.0.0+].
Result ommGetDefaultDisplayResolution(s32* width, s32* height);

/// Returns an OmmOperationMode.
Result ommGetOperationMode(OmmOperationMode* s);

/// Takes an OmmOperationModePolicy. Only available on [3.0.0+].
Result ommSetOperationModePolicy(OmmOperationModePolicy value);
