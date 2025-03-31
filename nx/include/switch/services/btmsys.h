/**
 * @file btmsys.h
 * @brief btm:sys (btm system) service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/btdrv_types.h"
#include "../services/btm_types.h"
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

/**
 * @brief StartAudioDeviceDiscovery
 * @note Only available on [13.0.0+].
 */
Result btmsysStartAudioDeviceDiscovery(void);

/**
 * @brief StopAudioDeviceDiscovery
 * @note Only available on [13.0.0+].
 */
Result btmsysStopAudioDeviceDiscovery(void);

/**
 * @brief IsDiscoveryingAudioDevice
 * @note Only available on [13.0.0+].
 * @param[out] out Output flag.
 */
Result btmsysIsDiscoveryingAudioDevice(bool *out);

/**
 * @brief GetDiscoveredAudioDevice
 * @note Only available on [13.0.0+].
 * @param[out] out Output array of \ref BtmAudioDevice.
 * @param[in] count Size of the out array in entries. The max is 15.
 * @param[out] total_out Total output entries.
 */
Result btmsysGetDiscoveredAudioDevice(BtmAudioDevice *out, s32 count, s32 *total_out);

/**
 * @brief AcquireAudioDeviceConnectionEvent
 * @note Only available on [13.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmsysAcquireAudioDeviceConnectionEvent(Event* out_event);

/**
 * @brief ConnectAudioDevice
 * @note Only available on [13.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btmsysConnectAudioDevice(BtdrvAddress addr);

/**
 * @brief IsConnectingAudioDevice
 * @note Only available on [13.0.0+].
 * @param[out] out Output flag.
 */
Result btmsysIsConnectingAudioDevice(bool *out);

/**
 * @brief GetConnectedAudioDevices
 * @note Only available on [13.0.0+].
 * @param[out] out Output array of \ref BtmAudioDevice.
 * @param[in] count Size of the out array in entries. The max is 8.
 * @param[out] total_out Total output entries.
 */
Result btmsysGetConnectedAudioDevices(BtmAudioDevice *out, s32 count, s32 *total_out);

/**
 * @brief DisconnectAudioDevice
 * @note Only available on [13.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btmsysDisconnectAudioDevice(BtdrvAddress addr);

/**
 * @brief AcquirePairedAudioDeviceInfoChangedEvent
 * @note Only available on [13.0.0+].
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result btmsysAcquirePairedAudioDeviceInfoChangedEvent(Event* out_event);

/**
 * @brief GetPairedAudioDevices
 * @note Only available on [13.0.0+].
 * @param[out] out Output array of \ref BtmAudioDevice.
 * @param[in] count Size of the out array in entries. The max is 10.
 * @param[out] total_out Total output entries.
 */
Result btmsysGetPairedAudioDevices(BtmAudioDevice *out, s32 count, s32 *total_out);

/**
 * @brief RemoveAudioDevicePairing
 * @note Only available on [13.0.0+].
 * @param[in] addr \ref BtdrvAddress
 */
Result btmsysRemoveAudioDevicePairing(BtdrvAddress addr);

/**
 * @brief RequestAudioDeviceConnectionRejection
 * @note Only available on [13.0.0+].
 */
Result btmsysRequestAudioDeviceConnectionRejection(void);

/**
 * @brief CancelAudioDeviceConnectionRejection
 * @note Only available on [13.0.0+].
 */
Result btmsysCancelAudioDeviceConnectionRejection(void);
