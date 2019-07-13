/**
 * @file wlaninf.h
 * @brief WLAN InfraManager service IPC wrapper.
 * @author natinusala
 * @copyright libnx Authors
 */

#pragma once
#include "../kernel/ipc.h"
#include "../services/sm.h"

/// WLAN State.
typedef enum {
    WlanInfState_NotConnected = 1, ///< WLAN is disabled or enabled and not connected.
    WlanInfState_Connecting,       ///< WLAN is connecting.
    WlanInfState_Connected,        ///< WLAN is connected.
} WlanInfState;

Result wlaninfInitialize(void);
void wlaninfExit(void);
Service* wlaninfGetServiceSession(void);

Result wlaninfGetState(WlanInfState* out);

/// Value goes from -30 (really good signal) to -90 (barely enough to stay connected)
/// on a logarithmic scale
Result wlaninfGetRSSI(s32* out);
