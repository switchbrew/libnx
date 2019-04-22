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

/// Confirm whether VrMode is allowed. Only available with [4.0.0+].
Result pctlConfirmStereoVisionPermission(void);

/// Gets whether Parental Controls are enabled.
Result pctlIsRestrictionEnabled(bool *flag);

/// Reset the confirmation done by \ref pctlConfirmStereoVisionPermission. Only available with [5.0.0+].
Result pctlResetConfirmedStereoVisionPermission(void);

/// Gets whether VrMode is allowed. Only available with [5.0.0+].
Result pctlIsStereoVisionPermitted(bool *flag);

