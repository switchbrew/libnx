/**
 * @file bpc.h
 * @brief Board power control (bpc) service IPC wrapper.
 * @author XorTroll, SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

typedef enum {
    BpcSleepButtonState_Held = 0,
    BpcSleepButtonState_Released = 1,
} BpcSleepButtonState;

Result bpcInitialize(void);
void bpcExit(void);

Result bpcShutdownSystem(void);
Result bpcRebootSystem(void);
Result bpcGetSleepButtonState(BpcSleepButtonState *out);
