/**
 * @file pctl.h
 * @brief Parental Controls service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../kernel/ipc.h"
#include "../services/sm.h"

Result pctlInitialize(void);
void pctlExit(void);

/// Gets whether Parental Controls are enabled.
Result pctlIsRestrictionEnabled(bool *flag);

