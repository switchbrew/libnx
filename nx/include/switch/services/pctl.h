/**
 * @file pctl.h
 * @brief Parental Controls service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"

/// Initialize pctl.
Result pctlInitialize(void);

/// Exit pctl.
void pctlExit(void);

/// Gets the Service object for the actual pctl service session.
Service* pctlGetServiceSession(void);

/// Gets the Service object for IParentalControlService.
Service* pctlGetServiceSession_Service(void);

/// Confirm whether VrMode is allowed. Only available with [4.0.0+].
Result pctlConfirmStereoVisionPermission(void);

/// Gets whether Parental Controls are enabled.
Result pctlIsRestrictionEnabled(bool *flag);

/// Reset the confirmation done by \ref pctlConfirmStereoVisionPermission. Only available with [5.0.0+].
Result pctlResetConfirmedStereoVisionPermission(void);

/// Gets whether VrMode is allowed. Only available with [5.0.0+].
Result pctlIsStereoVisionPermitted(bool *flag);

