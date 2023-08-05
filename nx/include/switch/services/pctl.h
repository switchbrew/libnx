/**
 * @file pctl.h
 * @brief Parental Controls service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

typedef struct {
    u8 rating_age;
    bool sns_post_restriction;
    bool free_communication_restriction;
} PctlRestrictionSettings;

/// Initialize pctl.
Result pctlInitialize(void);

/// Exit pctl.
void pctlExit(void);

/// Gets the Service object for the actual pctl service session.
Service* pctlGetServiceSession(void);

/// Gets the Service object for IParentalControlService.
Service* pctlGetServiceSession_Service(void);

/// Gets whether Parental Controls restrictions are temporarily unlocked.
Result pctlIsRestrictionTemporaryUnlocked(bool *flag);

/// Confirm whether VrMode is allowed. Only available with [4.0.0+].
Result pctlConfirmStereoVisionPermission(void);

/// Gets whether Parental Controls are enabled.
Result pctlIsRestrictionEnabled(bool *flag);

/// Gets whether Parental Controls are enabled.
Result pctlGetSafetyLevel(u32 *safety_level);

/// Returns the current restrictions settings.
Result pctlGetCurrentSettings(PctlRestrictionSettings *settings);

/// Gets the count of applications that have free communication.
Result pctlGetFreeCommunicationApplicationListCount(u32 *count);

/// Reset the confirmation done by \ref pctlConfirmStereoVisionPermission. Only available with [5.0.0+].
Result pctlResetConfirmedStereoVisionPermission(void);

/// Gets whether VrMode is allowed. Only available with [5.0.0+].
Result pctlIsStereoVisionPermitted(bool *flag);

/// Confirm whether pairing is active.
Result pctlIsPairingActive(bool *flag);

/// Returns the synchronization event.
Result pctlGetSynchronizationEvent(Event* out_event);

/// Returns the supension event.
Result pctlGetPlayTimerEventToRequestSuspension(Event* out_event);

/// Confirm whether play timer alarm is disabled. Only available with [4.0.0+].
Result pctlIsPlayTimerAlarmDisabled(bool *flag);

/// Returns the unlinked event.
Result pctlGetUnlinkedEvent(Event* out_event);
