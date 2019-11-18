/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet, exelix, yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"

typedef enum {
    NifmServiceType_User           = 0, ///< Initializes nifm:u.
    NifmServiceType_System         = 1, ///< Initializes nifm:s.
    NifmServiceType_Admin          = 2, ///< Initializes nifm:a.
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

/// Initialize nifm. This is used automatically by gethostid().
Result nifmInitialize(NifmServiceType service_type);

/// Exit nifm. This is used automatically by gethostid().
void nifmExit(void);

/// Gets the Service object for the actual nifm:* service session.
Service* nifmGetServiceSession_StaticService(void);

/// Gets the Service object for IGeneralService.
Service* nifmGetServiceSession_GeneralService(void);

Result nifmGetCurrentIpAddress(u32* out);

/**
 * @note Works only if called from nifm:a or nifm:s.
 */
Result nifmSetWirelessCommunicationEnabled(bool enable);

Result nifmIsWirelessCommunicationEnabled(bool* out);

/**
 * @note Will fail with 0xd46ed if Internet is neither connecting or connected (airplane mode or no known network in reach).
 * @param wifiStrength Strength of the Wi-Fi signal in number of bars from 0 to 3.
 */
Result nifmGetInternetConnectionStatus(NifmInternetConnectionType* connectionType, u32* wifiStrength, NifmInternetConnectionStatus* connectionStatus);

Result nifmIsEthernetCommunicationEnabled(bool* out);
Result nifmIsAnyForegroundRequestAccepted(bool* out);
Result nifmPutToSleep(void);
Result nifmWakeUp(void);
