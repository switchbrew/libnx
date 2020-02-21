/**
 * @file tc.h
 * @brief Temperature control (tc) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize tc.
Result tcInitialize(void);

/// Exit tc.
void tcExit(void);

/// Getc the Service for tc.
Service* tcGetServiceSession(void);

Result tcEnableFanControl(void);
Result tcDisableFanControl(void);
Result tcIsFanControlEnabled(bool *status);
/// Only available on [5.0.0+]
Result tcGetSkinTemperatureMilliC(s32 *skinTemp);

