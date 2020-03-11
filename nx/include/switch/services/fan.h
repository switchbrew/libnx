/**
 * @file fan.h
 * @brief Fan service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef struct {
    Service s;
} FanController;

/// Initialize fan.
Result fanInitialize(void);

/// Exit fan.
void fanExit(void);

/// Gets the Service object for the actual fan service session.
Service* fanGetServiceSession(void);

/// Opens IController session.
Result fanOpenController(FanController *out, u32 device_code);

/// Close IController session.
void fanControllerClose(FanController *controller);

/// @warning Disabling your fan can damage your system.
Result fanControllerSetRotationSpeedLevel(FanController *controller, float level);
Result fanControllerGetRotationSpeedLevel(FanController *controller, float *level);
