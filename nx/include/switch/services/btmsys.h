/**
 * @file btmsys.h
 * @brief btm:sys (btm system) service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

/// Initialize btm:sys.
Result btmsysInitialize(void);

/// Exit btm:sys.
void btmsysExit(void);

/// Gets the Service object for the actual btm:sys service session. This object must be closed by the user once finished using cmds with this.
Result btmsysGetServiceSession(Service* srv_out);

/// Gets the Service object for IBtmSystemCore.
Service* btmsysGetServiceSession_IBtmSystemCore(void);

/**
 * @brief StartGamepadPairing
 */
Result btmsysStartGamepadPairing(void);

/**
 * @brief CancelGamepadPairing
 */
Result btmsysCancelGamepadPairing(void);

/**
 * @brief ClearGamepadPairingDatabase
 */
Result btmsysClearGamepadPairingDatabase(void);

/**
 * @brief GetPairedGamepadCount
 * @param[out] out Output count.
 */
Result btmsysGetPairedGamepadCount(u8 *out);

/**
 * @brief EnableRadio
 */
Result btmsysEnableRadio(void);

/**
 * @brief DisableRadio
 */
Result btmsysDisableRadio(void);

/**
 * @brief GetRadioOnOff
 * @param[out] out Output flag.
 */
Result btmsysGetRadioOnOff(bool *out);

/**
 * @brief AcquireRadioEvent
 * @note Only available on [3.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmsysAcquireRadioEvent(Event* out_event);

/**
 * @brief AcquireGamepadPairingEvent
 * @note Only available on [3.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmsysAcquireGamepadPairingEvent(Event* out_event);

/**
 * @brief IsGamepadPairingStarted
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result btmsysIsGamepadPairingStarted(bool *out);

