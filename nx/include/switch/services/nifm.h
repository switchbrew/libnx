/**
 * @file nifm.h
 * @brief Network interface service IPC wrapper.
 * @author shadowninja108, shibboleet, exelix, yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../kernel/event.h"

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

typedef enum {
    NifmRequestState_Invalid                            = 0, ///< Error.
    NifmRequestState_Unknown1                           = 1, ///< Not yet submitted or error.
    NifmRequestState_OnHold                             = 2, ///< OnHold
    NifmRequestState_Available                          = 3, ///< Available
    NifmRequestState_Unknown4                           = 4, ///< Unknown
    NifmRequestState_Unknown5                           = 5, ///< Unknown
} NifmRequestState;

/// Request
typedef struct {
    Service s;                                           ///< IRequest
    Event event_request_state;                           ///< First Event from cmd GetSystemEventReadableHandles, autoclear=true. Signaled when the RequestState changes.
    Event event1;                                        ///< Second Event from cmd GetSystemEventReadableHandles.

    NifmRequestState request_state;                      ///< \ref NifmRequestState from the GetRequestState cmd.
    Result res;                                          ///< Result from the GetResult cmd.
} NifmRequest;

/// ClientId
typedef struct {
    u32 id;                                              ///< ClientId
} NifmClientId;

/// IpV4Address
typedef struct {
    u8 addr[4];                                          ///< IPv4 address, aka struct in_addr.
} NifmIpV4Address;

/// IpAddressSetting
typedef struct {
    u8 is_automatic;                                     ///< Whether this setting is automatic. Ignored by \ref nifmGetCurrentIpConfigInfo.

    NifmIpV4Address current_addr;                        ///< Current address.
    NifmIpV4Address subnet_mask;                         ///< Subnet Mask.
    NifmIpV4Address gateway;                             ///< Gateway.
} NifmIpAddressSetting;

/// DnsSetting
typedef struct {
    u8 is_automatic;                                     ///< Whether this setting is automatic. Ignored by \ref nifmGetCurrentIpConfigInfo.

    NifmIpV4Address primary_dns_server;                  ///< Primary DNS server.
    NifmIpV4Address secondary_dns_server;                ///< Secondary DNS server.
} NifmDnsSetting;

/// ProxySetting
typedef struct {
    u8 enabled;                                          ///< Enables using the proxy when set.
    u8 pad;                                              ///< Padding
    u16 port;                                            ///< Port
    char server[0x64];                                   ///< Server string, NUL-terminated.
    u8 auto_auth_enabled;                                ///< Enables auto-authentication when set, which uses the following two strings.
    char user[0x20];                                     ///< User string, NUL-terminated.
    char password[0x20];                                 ///< Password string, NUL-terminated.
    u8 pad2;                                             ///< Padding
} NifmProxySetting;

/// IpSettingData
typedef struct {
    NifmIpAddressSetting ip_address_setting;             ///< \ref NifmIpAddressSetting
    NifmDnsSetting dns_setting;                          ///< \ref NifmDnsSetting
    NifmProxySetting proxy_setting;                      ///< \ref NifmProxySetting
    u16 mtu;                                             ///< MTU
} NifmIpSettingData;

/// WirelessSettingData
typedef struct {
    u8 ssid_len;                                         ///< NifmSfWirelessSettingData::ssid_len
    char ssid[0x21];                                     ///< NifmSfWirelessSettingData::ssid
    u8 unk_x22;                                          ///< NifmSfWirelessSettingData::unk_x21
    u8 pad;                                              ///< Padding
    u32 unk_x24;                                         ///< NifmSfWirelessSettingData::unk_x22
    u32 unk_x28;                                         ///< NifmSfWirelessSettingData::unk_x23
    u8 passphrase[0x41];                                 ///< NifmSfWirelessSettingData::passphrase
    u8 pad2[0x3];                                        ///< Padding
} NifmWirelessSettingData;

/// SfWirelessSettingData
typedef struct {
    u8 ssid_len;                                         ///< SSID length.
    char ssid[0x20];                                     ///< SSID string.
    u8 unk_x21;                                          ///< Unknown
    u8 unk_x22;                                          ///< Unknown
    u8 unk_x23;                                          ///< Unknown
    u8 passphrase[0x41];                                 ///< Passphrase
} NifmSfWirelessSettingData;

/// SfNetworkProfileData. Converted to/from \ref NifmNetworkProfileData.
typedef struct {
    NifmIpSettingData ip_setting_data;                   ///< \ref NifmIpSettingData
    Uuid uuid;                                           ///< Uuid
    char network_name[0x40];                             ///< NUL-terminated Network Name string.
    u8 unk_x112;                                         ///< Unknown
    u8 unk_x113;                                         ///< Unknown
    u8 unk_x114;                                         ///< Unknown
    u8 unk_x115;                                         ///< Unknown
    NifmSfWirelessSettingData wireless_setting_data;     ///< \ref NifmSfWirelessSettingData
    u8 pad;                                              ///< Padding
} NifmSfNetworkProfileData;

/// NetworkProfileData. Converted from/to \ref NifmSfNetworkProfileData.
typedef struct {
    Uuid uuid;                                           ///< NifmSfNetworkProfileData::uuid
    char network_name[0x40];                             ///< NifmSfNetworkProfileData::network_name
    u32 unk_x50;                                         ///< NifmSfNetworkProfileData::unk_x112
    u32 unk_x54;                                         ///< NifmSfNetworkProfileData::unk_x113
    u8 unk_x58;                                          ///< NifmSfNetworkProfileData::unk_x114
    u8 unk_x59;                                          ///< NifmSfNetworkProfileData::unk_x115
    u8 pad[2];                                           ///< Padding
    NifmWirelessSettingData wireless_setting_data;       ///< \ref NifmWirelessSettingData
    NifmIpSettingData ip_setting_data;                   ///< \ref NifmIpSettingData
} NifmNetworkProfileData;

/// Initialize nifm. This is used automatically by gethostid().
Result nifmInitialize(NifmServiceType service_type);

/// Exit nifm. This is used automatically by gethostid().
void nifmExit(void);

/// Gets the Service object for the actual nifm:* service session.
Service* nifmGetServiceSession_StaticService(void);

/// Gets the Service object for IGeneralService.
Service* nifmGetServiceSession_GeneralService(void);

/**
 * @brief GetClientId
 */
NifmClientId nifmGetClientId(void);

/**
 * @brief CreateRequest
 * @param[out] r \ref NifmRequest
 * @param[in] autoclear Event autoclear to use for NifmRequest::event1, a default of true can be used for this.
 */
Result nifmCreateRequest(NifmRequest* r, bool autoclear);

/**
 * @brief GetCurrentNetworkProfile
 * @param[out] profile \ref NifmNetworkProfileData
 */
Result nifmGetCurrentNetworkProfile(NifmNetworkProfileData *profile);

/**
 * @brief GetNetworkProfile
 * @param[in] uuid Uuid
 * @param[out] profile \ref NifmNetworkProfileData
 */
Result nifmGetNetworkProfile(Uuid uuid, NifmNetworkProfileData *profile);

/**
 * @brief SetNetworkProfile
 * @note Only available with ::NifmServiceType_Admin.
 * @param[in] profile \ref NifmNetworkProfileData
 * @param[out] uuid Uuid
 */
Result nifmSetNetworkProfile(const NifmNetworkProfileData *profile, Uuid *uuid);

/**
 * @brief GetCurrentIpAddress
 * @param[out] out IPv4 address (struct in_addr).
 */
Result nifmGetCurrentIpAddress(u32* out);

/**
 * @brief GetCurrentIpConfigInfo
 * @param[out] current_addr Same as \ref nifmGetCurrentIpAddress output.
 * @param[out] subnet_mask Subnet Mask (struct in_addr).
 * @param[out] gateway Gateway (struct in_addr).
 * @param[out] primary_dns_server Primary DNS server IPv4 address (struct in_addr).
 * @param[out] secondary_dns_server Secondary DNS server IPv4 address (struct in_addr).
 */
Result nifmGetCurrentIpConfigInfo(u32 *current_addr, u32 *subnet_mask, u32 *gateway, u32 *primary_dns_server, u32 *secondary_dns_server);

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

/**
 * @brief IsAnyInternetRequestAccepted
 * @param[in] id \ref NifmClientId
 */
bool nifmIsAnyInternetRequestAccepted(NifmClientId id);

Result nifmIsAnyForegroundRequestAccepted(bool* out);
Result nifmPutToSleep(void);
Result nifmWakeUp(void);

/**
 * @brief SetWowlDelayedWakeTime
 * @note Only available with ::NifmServiceType_System or ::NifmServiceType_Admin.
 * @note Only available on [9.0.0+].
 * @param[in] val Input value.
 */
Result nifmSetWowlDelayedWakeTime(s32 val);

///@name IRequest
///@{

/**
 * @brief Close a \ref NifmRequest.
 * @param r \ref NifmRequest
 */
void nifmRequestClose(NifmRequest* r);

/**
 * @brief GetRequestState
 * @param r \ref NifmRequest
 * @param[out] out \ref NifmRequestState
 */
Result nifmGetRequestState(NifmRequest* r, NifmRequestState *out);

/**
 * @brief GetResult
 * @param r \ref NifmRequest
 */
Result nifmGetResult(NifmRequest* r);

/**
 * @brief Cancel
 * @param r \ref NifmRequest
 */
Result nifmRequestCancel(NifmRequest* r);

/**
 * @brief Submit
 * @param r \ref NifmRequest
 */
Result nifmRequestSubmit(NifmRequest* r);

/**
 * @brief SubmitAndWait
 * @param r \ref NifmRequest
 */
Result nifmRequestSubmitAndWait(NifmRequest* r);

/**
 * @brief GetAppletInfo
 * @note This is used by \ref nifmLaHandleNetworkRequestResult.
 * @param r \ref NifmRequest
 * @param[in] theme_color ThemeColor
 * @param[out] buffer Output buffer for storage data.
 * @param[in] size Output buffer size.
 * @param[out] applet_id \ref AppletId
 * @param[out] mode \ref LibAppletMode
 * @param[out] out_size Total data size written to the output buffer.
 */
Result nifmRequestGetAppletInfo(NifmRequest* r, u32 theme_color, void* buffer, size_t size, u32 *applet_id, u32 *mode, u32 *out_size);

/**
 * @brief SetKeptInSleep
 * @note Only available on [3.0.0+].
 * @note ::NifmRequestState must be ::NifmRequestState_Unknown1.
 * @param r \ref NifmRequest
 * @param[in] flag Flag
 */
Result nifmRequestSetKeptInSleep(NifmRequest* r, bool flag);

/**
 * @brief RegisterSocketDescriptor. Only 1 socket can be registered at a time with a NifmRequest. Do not use directly, use \ref socketNifmRequestRegisterSocketDescriptor instead.
 * @note Only available on [3.0.0+].
 * @note ::NifmRequestState must be ::NifmRequestState_Available.
 * @param r \ref NifmRequest
 * @param[in] sockfd Socket fd
 */
Result nifmRequestRegisterSocketDescriptor(NifmRequest* r, int sockfd);

/**
 * @brief UnregisterSocketDescriptor. Do not use directly, use \ref socketNifmRequestUnregisterSocketDescriptor instead.
 * @note Only available on [3.0.0+].
 * @note ::NifmRequestState must be ::NifmRequestState_Available.
 * @param r \ref NifmRequest
 * @param[in] sockfd Socket fd, must match the fd previously registered with \ref nifmRequestRegisterSocketDescriptor.
 */
Result nifmRequestUnregisterSocketDescriptor(NifmRequest* r, int sockfd);

///@}

