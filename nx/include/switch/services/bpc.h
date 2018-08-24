/**
 * @file bpc.h
 * @brief Board power control (bpc) service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

Result bpcInitialize(void);
void bpcExit(void);

Result bpcShutdownSystem(void);
Result bpcRebootSystem(void);
