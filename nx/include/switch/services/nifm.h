/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet, exelix
 * @copyright libnx Authors
 */

#pragma once
#include "../kernel/ipc.h"
#include "../services/sm.h"

typedef enum {
    NifmServiceType_NotInitialized = 0, ///< Initializes nifm:u.
    NifmServiceType_User           = 1, ///< Initializes nifm:u.
    NifmServiceType_System         = 2, ///< Initializes nifm:s.
    NifmServiceType_Admin          = 3, ///< Initializes nifm:a.
} NifmServiceType;

typedef enum {
    NifmInternetConnectionType_WiFi     = 1, ///< Wi-Fi connection is used.
    NifmInternetConnectionType_Ethernet = 2, ///< Ethernet connection is used.
} NifmInternetConnectionType;

typedef enum {
    NifmInternetConnectionStatus_ConnectingUnknown1     = 0, ///< Unknown internet connection status 1.
    NifmInternetConnectionStatus_ConnectingUnknown2     = 1, ///< Unknown internet connection status 2.
    NifmInternetConnectionStatus_ConnectingUnknown3     = 2, ///< Unknown internet connection status 3 (conntest?).
    NifmInternetConnectionStatus_ConnectingUnknown4     = 3, ///< Unknown internet connection status 4.
    NifmInternetConnectionStatus_Connected              = 4, ///< Internet is connected.
} NifmInternetConnectionStatus;

/**
 * @brief Sets the \ref NifmServiceType for initialization. Call this function before \ref nifmInitialize.
 * @note By default nifm:u will be used.
 */
void nifmSetServiceType(NifmServiceType serviceType);

Result nifmInitialize(void);
void nifmExit(void);

Result nifmGetCurrentIpAddress(u32* out);

Result nifmIsWirelessCommunicationEnabled(bool* out);

/**
 * @note Works only if called from nifm:a or nifm:s.
 */
Result nifmSetWirelessCommunicationEnabled(bool enable);

/**
 * @note Will fail with 0xd46ed if Internet is neither connecting or connected (airplane mode or no known network in reach).
 * @param wifiStrength Strength of the Wi-Fi signal in number of bars from 0 to 3.
 */
Result nifmGetInternetConnectionStatus(NifmInternetConnectionType* connectionType, u32* wifiStrength, NifmInternetConnectionStatus* connectionStatus);

Result nifmIsEthernetCommunicationEnabled(bool* out);
Result nifmIsAnyForegroundRequestAccepted(bool* out);
Result nifmPutToSleep(void);
Result nifmWakeUp(void);
