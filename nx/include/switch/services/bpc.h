/**
 * @file bpc.h
 * @brief Board power control (bpc) service IPC wrapper.
 * @author XorTroll, SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    BpcSleepButtonState_Held = 0,
    BpcSleepButtonState_Released = 1,
} BpcSleepButtonState;

/// Initialize bpc.
Result bpcInitialize(void);

/// Exit bpc.
void bpcExit(void);

/// Gets the Service object for the actual bpc service session.
Service* bpcGetServiceSession(void);

Result bpcShutdownSystem(void);
Result bpcRebootSystem(void);
Result bpcGetSleepButtonState(BpcSleepButtonState *out);
