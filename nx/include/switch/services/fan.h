#pragma once
#include "../types.h"
#include "../sf/service.h"

Result fanInitialize(void);
void fanExit(void);
Service* fanGetServiceSession(void);
Service* fanGetServiceSession_Controller(void);

Result fanSetRotationSpeedLevel(float level);
Result fanGetRotationSpeedLevel(float *level);