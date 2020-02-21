/**
 * @file fan.h
 * @brief Fan service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize fan.
Result fanInitialize(void);

/// Exit fan.
void fanExit(void);

/// Gets the Service object for the actual fan service session.
Service* fanGetServiceSession(void);

/// Gets the Service object for the actual fan controller service session.
Service* fanGetServiceSession_Controller(void);

Result fanSetRotationSpeedLevel(float level);
Result fanGetRotationSpeedLevel(float *level);