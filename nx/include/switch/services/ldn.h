/**
 * @file ldn.h
 * @brief LDN (local network communications) IPC wrapper. See also: https://switchbrew.org/wiki/LDN_services
 * @author yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../kernel/event.h"

#define LDN_PRIORITY_SYSTEM 0x38       ///< System priority for __nx_ldn_priority.
#define LDN_PRIORITY_USER 0x5A         ///< User priority for __nx_ldn_priority.

typedef enum {
    LdnServiceType_User           = 0, ///< Initializes ldn:u.
    LdnServiceType_System         = 1, ///< Initializes ldn:s.
} LdnServiceType;

/// State loaded by \ref ldnmGetStateForMonitor / \ref ldnGetState.
typedef enum {
    LdnState_None                 = 0, ///< None
    LdnState_Initialized          = 1, ///< Initialized
    LdnState_AccessPoint          = 2, ///< AccessPoint (\ref ldnOpenAccessPoint)
    LdnState_AccessPointCreated   = 3, ///< AccessPointCreated (\ref ldnCreateNetwork / \ref ldnCreateNetworkPrivate)
    LdnState_Station              = 4, ///< Station (\ref ldnOpenStation)
    LdnState_StationConnected     = 5, ///< StationConnected (\ref ldnConnect / \ref ldnConnectPrivate)
    LdnState_Error                = 6, ///< Error
} LdnState;

/// DisconnectReason loaded by \ref ldnGetDisconnectReason.
typedef enum {
    LdnDisconnectReason_None                     = 0, ///< None
    LdnDisconnectReason_DisconnectedByUser       = 1, ///< DisconnectedByUser
    LdnDisconnectReason_DisconnectedBySystem     = 2, ///< DisconnectedBySystem
    LdnDisconnectReason_DestroyedByUser          = 3, ///< DestroyedByUser
    LdnDisconnectReason_DestroyedBySystem        = 4, ///< DestroyedBySystem
    LdnDisconnectReason_Rejected                 = 5, ///< Rejected
    LdnDisconnectReason_SignalLost               = 6, ///< SignalLost
} LdnDisconnectReason;

/// ScanFilterFlag
typedef enum {
    LdnScanFilterFlag_LocalCommunicationId      = BIT(0),                                                               ///< Enables using LdnScanFilter::network_id LdnIntentId::local_communication_id.
    LdnScanFilterFlag_SessionId                 = BIT(1),                                                               ///< Enables using LdnScanFilter::network_id session_id.
    LdnScanFilterFlag_NetworkType               = BIT(2),                                                               ///< Enables using LdnScanFilter::network_type.
    LdnScanFilterFlag_Bssid                     = BIT(3),                                                               ///< Enables using LdnScanFilter::bssid. Only available with \ref ldnScanPrivate.
    LdnScanFilterFlag_Ssid                      = BIT(4),                                                               ///< Enables using LdnScanFilter::ssid.
    LdnScanFilterFlag_SceneId                   = BIT(5),                                                               ///< Enables using LdnScanFilter::network_id LdnIntentId::scene_id.
    LdnScanFilterFlag_IntentId                  = LdnScanFilterFlag_LocalCommunicationId | LdnScanFilterFlag_SceneId,   ///< Enables using LdnScanFilter::network_id intent_id.
    LdnScanFilterFlag_NetworkId                 = LdnScanFilterFlag_IntentId | LdnScanFilterFlag_SessionId,             ///< Enables using LdnScanFilter::network_id.
} LdnScanFilterFlag;

/// AcceptPolicy
typedef enum {
    LdnAcceptPolicy_AlwaysAccept   = 0, ///< AlwaysAccept
    LdnAcceptPolicy_AlwaysReject   = 1, ///< AlwaysReject
    LdnAcceptPolicy_BlackList      = 2, ///< BlackList, addresses in the list (\ref ldnAddAcceptFilterEntry) are not allowed.
    LdnAcceptPolicy_WhiteList      = 3, ///< WhiteList, only addresses in the list (\ref ldnAddAcceptFilterEntry) are allowed.
} LdnAcceptPolicy;

/// SecurityMode. LdnSecurityMode_Product can be used as the default value. The ldn cmds which use an input SecurityMode normally overwrite the SecurityMode.
typedef enum {
    LdnSecurityMode_Any            = 0, ///< Any
    LdnSecurityMode_Product        = 1, ///< Product
    LdnSecurityMode_Debug          = 2, ///< Debug
    LdnSecurityMode_SystemDebug    = 3, ///< SystemDebug
} LdnSecurityMode;

/// OperationMode
typedef enum {
    LdnOperationMode_Stable        = 0, ///< Stable
    LdnOperationMode_HighSpeed     = 1, ///< HighSpeed
} LdnOperationMode;

/// WirelessControllerRestriction
typedef enum {
    LdnWirelessControllerRestriction_Disabled = 0,   ///< Disabled
    LdnWirelessControllerRestriction_Enabled  = 1,   ///< Enabled
} LdnWirelessControllerRestriction;

/// Protocol
typedef enum {
    LdnProtocol_NX       = 1,   ///< NX (default)
    LdnProtocol_Unknown3 = 3,   ///< (NXAndOunce?)
} LdnProtocol;

/// Ipv4Address. This is essentially the same as struct in_addr - hence this can be used with standard sockets (byteswap required).
typedef struct {
    u32 addr;                          ///< Address
} LdnIpv4Address;

/// SubnetMask. This is essentially the same as struct in_addr - hence this can be used with standard sockets (byteswap required).
typedef struct {
    u32 mask;                          ///< Mask
} LdnSubnetMask;

/// MacAddress
typedef struct {
    u8 addr[6];                        ///< Address
} LdnMacAddress;

/// Ssid
typedef struct {
    u8 len;                            ///< Length excluding NUL-terminator, must be 0x1-0x20.
    char str[0x21];                    ///< SSID string including NUL-terminator, str[len_field] must be 0. The chars in this string must be be in the range of 0x20-0x7F, for when the Ssid is converted to a string (otherwise the byte written to the string will be 0).
} LdnSsid;

/// NodeLatestUpdate
typedef struct {
    u8 state_change;                   ///< StateChange. The field in state is reset to zero by \ref ldnGetNetworkInfoLatestUpdate after loading it.
    u8 reserved[0x7];                  ///< Reserved
} LdnNodeLatestUpdate;

/// AddressEntry
typedef struct {
    LdnIpv4Address ip_addr;            ///< \ref LdnIpv4Address
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress
    u8 reserved[0x2];                  ///< Reserved
} LdnAddressEntry;

/// NodeInfo
typedef struct {
    LdnIpv4Address ip_addr;            ///< \ref LdnIpv4Address
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress
    s8 node_id;                        ///< NodeId
    u8 is_connected;                   ///< IsConnected flag
    char user_name[0x21];              ///< LdnUserConfig::user_name
    u8 platform;                       ///< [19.0.0+] Platform? (0 = NX, 1 = Ounce)
    s16 local_communication_version;   ///< LocalCommunicationVersion
    u8 reserved_x30[0x10];             ///< Reserved
} LdnNodeInfo;

/// UserConfig. The input struct is copied to a tmp struct, which is then used with the cmd.
typedef struct {
    char user_name[0x21];             ///< UserName (NUL-terminated string for the user name)
    u8 reserved[0xF];                 ///< Reserved
} LdnUserConfig;

/// IntentId
typedef struct {
    s64 local_communication_id;        ///< LocalCommunicationId. \ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnect, \ref ldnConnectPrivate (also \ref LdnScanFilter when enabled): When -1, this is overwritten with the first LocalCommunicationId from the user-process NACP, if loading fails value 0 is written instead. Otherwise when not -1, if NACP loading is successful, this field must match one of the LocalCommunicationIds from there.
    u8 reserved_x8[0x2];               ///< Reserved
    u16 scene_id;                      ///< SceneId. Arbitrary user data, this can be used for filtering with \ref LdnScanFilter (::LdnScanFilterFlag_SceneId) for example.
    u8 reserved_xC[0x4];               ///< Reserved
} LdnIntentId;

/// SessionId
typedef struct {
    u8 random[0x10];                   ///< Random. Used to generate/overwrite the ssid when needed.
} LdnSessionId;

/// NetworkId
typedef struct {
    LdnIntentId intent_id;             ///< IntentId
    LdnSessionId session_id;           ///< SessionId
} LdnNetworkId;

/// CommonNetworkInfo
typedef struct {
    LdnMacAddress bssid;               ///< \ref LdnMacAddress
    LdnSsid ssid;                      ///< \ref LdnSsid
    s16 channel;                       ///< Channel
    s8 link_level;                     ///< LinkLevel
    u8 network_type;                   ///< NetworkType
    u8 reserved[0x4];                  ///< Reserved
} LdnCommonNetworkInfo;

/// NetworkInfo
typedef struct {
    LdnNetworkId network_id;           ///< NetworkId
    LdnCommonNetworkInfo common;       ///< CommonNetworkInfo
    u8 server_random[0x10];            ///< LdnSecurityParameter::server_random
    u16 security_mode;                 ///< LdnSecurityConfig::security_mode
    u8 station_accept_policy;          ///< \ref LdnAcceptPolicy
    u8 version;                        ///< Version
    u8 reserved_x14[0x2];              ///< Reserved
    s8 node_count_max;                 ///< NodeCountMax
    u8 node_count;                     ///< NodeCount, number of set entries in nodes.
    LdnNodeInfo nodes[8];              ///< Array of \ref LdnNodeInfo, starting with the AccessPoint node.
    u8 reserved_x218[0x2];             ///< Reserved
    u16 advertise_data_size;           ///< AdvertiseDataSize (\ref ldnSetAdvertiseData)
    u8 advertise_data[0x180];          ///< AdvertiseData (\ref ldnSetAdvertiseData)
    u8 reserved_x39C[0x8C];            ///< Reserved
    u64 reserved_x428;                 ///< Reserved
} LdnNetworkInfo;

/// ScanFilter. \ref ldnScan / \ref ldnScanPrivate will only return the \ref LdnNetworkInfo when validating the \ref LdnNetworkInfo with LdnScanFilter was sucessful, with the fields enabled by \ref LdnScanFilterFlag.
typedef struct {
    LdnNetworkId network_id;           ///< NetworkId
    u32 network_type;                  ///< NetworkType (::LdnScanFilterFlag_NetworkType)
    LdnMacAddress bssid;               ///< Bssid (::LdnScanFilterFlag_Bssid)
    LdnSsid ssid;                      ///< Ssid (::LdnScanFilterFlag_Ssid)
    u8 reserved[0x10];                 ///< Reserved
    u32 flags;                         ///< Bitmask for \ref LdnScanFilterFlag.
} LdnScanFilter;

/// SecurityConfig
typedef struct {
    u16 security_mode;                 ///< \ref LdnSecurityMode Overwritten by \ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnect, \ref ldnConnectPrivate.
    u16 passphrase_size;               ///< PassphraseSize. Must be 0x10-0x40.
    u8 passphrase[0x40];               ///< Passphrase, used with key derivation.
} LdnSecurityConfig;

/// SecurityParameter. The struct used by \ref ldnCreateNetwork internally is randomly-generated.
typedef struct {
    u8 server_random[0x10];            ///< ServerRandom, used with the same key derivation as \ref LdnSecurityConfig.
    LdnSessionId session_id;           ///< SessionId
} LdnSecurityParameter;

/// NetworkConfig. The input struct is copied to a tmp struct, which is then used with the cmd (\ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnectPrivate).
typedef struct {
    LdnIntentId intent_id;             ///< IntentId
    s16 channel;                       ///< LdnCommonNetworkInfo::channel. Channel, can be zero. Overwritten internally by \ref ldnCreateNetwork.
    s8 node_count_max;                 ///< LdnNetworkInfo::node_count_max. \ref ldnCreateNetwork / \ref ldnCreateNetworkPrivate: Must be 0x1-0x8.
    u8 reserved_x13;                   ///< Cleared to zero for the tmp struct.
    s16 local_communication_version;   ///< LdnNodeInfo::local_communication_version, for the first entry in LdnNetworkInfo::nodes. Must not be negative.
    u8 reserved_x16[0xA];              ///< Cleared to zero for the tmp struct.
} LdnNetworkConfig;

/// ActionFrameSettings
typedef struct {
    s64 local_communication_id;        ///< LocalCommunicationId (Same handling as LdnIntentId::local_communication_id)
    u8 reserved[0x34];                 ///< Reserved
    u16 security_mode;                 ///< \ref LdnSecurityMode (Must be ::LdnSecurityMode_Product or ::LdnSecurityMode_Debug, internally this is overriden)
    u16 passphrase_size;               ///< PassphraseSize (Must be 0x10-0x40)
    u8 passphrase[0x40];               ///< Passphrase
} LdnActionFrameSettings;

///@name ldn:m
///@{

/// Initialize ldn:m.
Result ldnmInitialize(void);

/// Exit ldn:m.
void ldnmExit(void);

/// Gets the Service object for IMonitorService.
Service* ldnmGetServiceSession_MonitorService(void);

/**
 * @brief GetState
 * @param[out] out \ref LdnState
 */
Result ldnmGetState(LdnState *out);

/**
 * @brief GetNetworkInfo
 * @param[out] out \ref LdnNetworkInfo
 */
Result ldnmGetNetworkInfo(LdnNetworkInfo *out);

/**
 * @brief GetIpv4Address
 * @param[out] addr \ref LdnIpv4Address
 * @param[out] mask \ref LdnSubnetMask
 */
Result ldnmGetIpv4Address(LdnIpv4Address *addr, LdnSubnetMask *mask);

/**
 * @brief GetSecurityParameter
 * @note Not exposed by official sw.
 * @param[out] out \ref LdnSecurityParameter
 */
Result ldnmGetSecurityParameter(LdnSecurityParameter *out);

/**
 * @brief GetNetworkConfig
 * @note Not exposed by official sw.
 * @param[out] out \ref LdnNetworkConfig
 */
Result ldnmGetNetworkConfig(LdnNetworkConfig *out);

///@}

///@name ldn
///@{

/// Initialize ldn.
/// The priority is only used with ::LdnServiceType_System on [19.0.0+]. To optionally set this, define "s32 __nx_ldn_priority". The priority must be \ref LDN_PRIORITY_SYSTEM (default) or \ref LDN_PRIORITY_USER.
Result ldnInitialize(LdnServiceType service_type);

/// Exit ldn.
void ldnExit(void);

/// Gets the Service object for IUserLocalCommunicationService/ISystemLocalCommunicationService.
Service* ldnGetServiceSession_LocalCommunicationService(void);

/// Gets the Service object for IClientProcessMonitor, only valid with [18.0.0+].
Service* ldnGetServiceSession_IClientProcessMonitor(void);

/**
 * @brief GetState
 * @param[out] out \ref LdnState
 */
Result ldnGetState(LdnState *out);

/**
 * @brief GetNetworkInfo
 * @param[out] out \ref LdnNetworkInfo
 */
Result ldnGetNetworkInfo(LdnNetworkInfo *out);

/**
 * @brief GetIpv4Address
 * @param[out] addr \ref LdnIpv4Address
 * @param[out] mask \ref LdnSubnetMask
 */
Result ldnGetIpv4Address(LdnIpv4Address *addr, LdnSubnetMask *mask);

/**
 * @brief GetDisconnectReason
 * @param[out] out \ref LdnDisconnectReason
 */
Result ldnGetDisconnectReason(LdnDisconnectReason *out);

/**
 * @brief GetSecurityParameter
 * @param[out] out \ref LdnSecurityParameter
 */
Result ldnGetSecurityParameter(LdnSecurityParameter *out);

/**
 * @brief GetNetworkConfig
 * @param[out] out \ref LdnNetworkConfig
 */
Result ldnGetNetworkConfig(LdnNetworkConfig *out);

/**
 * @brief GetStateChangeEvent
 * @note The Event must be closed by the user once finished with it.
 * @note This is signaled when the data returned by \ref ldnGetNetworkInfo / \ref ldnGetNetworkInfoLatestUpdate is updated.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result ldnGetStateChangeEvent(Event* out_event);

/**
 * @brief GetNetworkInfoAndHistory
 * @param[out] network_info \ref LdnNetworkInfo
 * @param[out] nodes Output array of \ref LdnNodeLatestUpdate.
 * @param[in] count Size of the nodes array in entries, must be 8.
 */
Result ldnGetNetworkInfoAndHistory(LdnNetworkInfo *network_info, LdnNodeLatestUpdate *nodes, s32 count);

/**
 * @brief Scan
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, ::LdnState_Station, or ::LdnState_StationConnected.
 * @note This is the same as \ref ldnScanPrivate (minus the masking for LdnScanFilter::flags), except this has the same channel-override functionality as \ref ldnCreateNetwork.
 * @param[in] channel Channel, value 0 can be used for this.
 * @param[in] filter \ref LdnScanFilter
 * @param[out] network_info Output array of \ref LdnNetworkInfo.
 * @param[in] count Size of the network_info array in entries. Must be at least 1, this is clamped to a maximum of 0x18 internally.
 * @param[out] total_out Total output entries.
 */
Result ldnScan(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out);

/**
 * @brief ScanPrivate
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, ::LdnState_Station, or ::LdnState_StationConnected.
 * @note See \ref ldnScan.
 * @param[in] channel Channel, value 0 can be used for this.
 * @param[in] filter \ref LdnScanFilter
 * @param[out] network_info Output array of \ref LdnNetworkInfo.
 * @param[in] count Size of the network_info array in entries. Must be at least 1, this is clamped to a maximum of 0x18 internally.
 * @param[out] total_out Total output entries.
 */
Result ldnScanPrivate(s32 channel, const LdnScanFilter *filter, LdnNetworkInfo *network_info, s32 count, s32 *total_out);

/**
 * @brief SetWirelessControllerRestriction
 * @note Only available on [5.0.0+].
 * @note \ref LdnState must be ::LdnState_Initialized.
 * @param[in] restriction \ref LdnWirelessControllerRestriction
 */
Result ldnSetWirelessControllerRestriction(LdnWirelessControllerRestriction restriction);

/**
 * @brief SetProtocol
 * @note This is only usable with [20.0.0+] (with [18.0.0-19-0.1] this is available but not usable).
 * @note \ref LdnState must be ::LdnState_Initialized. If a non-default Protocol is wanted, use this after \ref ldnInitialize.
 * @note This is used by \ref ldnInitialize with LdnProtocol_NX on [20.0.0+].
 * @param[in] protocol \ref LdnProtocol
 */
Result ldnSetProtocol(LdnProtocol protocol);

/**
 * @brief OpenAccessPoint
 * @note \ref LdnState must be ::LdnState_Initialized, this eventually sets the State to ::LdnState_AccessPoint.
 */
Result ldnOpenAccessPoint(void);

/**
 * @brief CloseAccessPoint
 * @note \ref LdnState must be ::LdnState_AccessPoint or ::LdnState_AccessPointCreated, this eventually sets the State to ::LdnState_Initialized.
 * @note Used automatically internally by \ref ldnExit if needed.
 */
Result ldnCloseAccessPoint(void);

/**
 * @brief CreateNetwork
 * @note \ref LdnState must be ::LdnState_AccessPoint, this eventually sets the State to ::LdnState_AccessPointCreated.
 * @param[in] sec_config \ref LdnSecurityConfig
 * @param[in] user_config \ref LdnUserConfig
 * @param[in] network_config \ref LdnNetworkConfig
 */
Result ldnCreateNetwork(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config);

/**
 * @brief CreateNetworkPrivate
 * @note \ref LdnState must be ::LdnState_AccessPoint, this eventually sets the State to ::LdnState_AccessPointCreated.
 * @note This is the same as \ref ldnCreateNetwork besides the additional user-specified params, and with this cmd LdnNetworkConfig::channel is not overwritten (unlike \ref ldnCreateNetwork).
 * @param[in] sec_config \ref LdnSecurityConfig
 * @param[in] sec_param \ref LdnSecurityParameter
 * @param[in] user_config \ref LdnUserConfig
 * @param[in] network_config \ref LdnNetworkConfig
 * @param[in] addrs Input array of \ref LdnAddressEntry. This can be NULL.
 * @param[in] count Size of the addrs array in entries. This must be <=8. This can be 0, in which case the network will be non-Private like \ref ldnCreateNetwork.
 */
Result ldnCreateNetworkPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config, const LdnAddressEntry *addrs, s32 count);

/**
 * @brief DestroyNetwork
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, this eventually sets the State to ::LdnState_AccessPoint.
 */
Result ldnDestroyNetwork(void);

/**
 * @brief Reject
 * @note \ref LdnState must be ::LdnState_AccessPointCreated.
 * @param[in] addr \ref LdnIpv4Address
 */
Result ldnReject(LdnIpv4Address addr);

/**
 * @brief SetAdvertiseData
 * @note An empty buffer (buffer=NULL/size=0) can be used to reset the AdvertiseData size in state to zero.
 * @note \ref LdnState must be ::LdnState_AccessPoint or ::LdnState_AccessPointCreated.
 * @param[in] buffer Input buffer containing arbitrary user data.
 * @param[in] size Input buffer size, must be <=0x180.
 */
Result ldnSetAdvertiseData(const void* buffer, size_t size);

/**
 * @brief SetStationAcceptPolicy
 * @note \ref LdnState must be ::LdnState_AccessPoint or ::LdnState_AccessPointCreated.
 * @param[in] policy \ref LdnAcceptPolicy
 */
Result ldnSetStationAcceptPolicy(LdnAcceptPolicy policy);

/**
 * @brief AddAcceptFilterEntry
 * @note \ref LdnState must be ::LdnState_AccessPoint or ::LdnState_AccessPointCreated.
 * @note See \ref LdnAcceptPolicy.
 * @param[in] addr \ref LdnMacAddress. If you want, you can also pass LdnNodeInfo::mac_addr for this.
 */
Result ldnAddAcceptFilterEntry(LdnMacAddress addr);

/**
 * @brief ClearAcceptFilter
 * @note \ref LdnState must be ::LdnState_AccessPoint or ::LdnState_AccessPointCreated.
 */
Result ldnClearAcceptFilter(void);

/**
 * @brief OpenStation
 * @note \ref LdnState must be ::LdnState_Initialized, this eventually sets the State to ::LdnState_Station.
 */
Result ldnOpenStation(void);

/**
 * @brief CloseStation
 * @note \ref LdnState must be ::LdnState_Station or ::LdnState_StationConnected, this eventually sets the State to ::LdnState_Initialized.
 * @note Used automatically internally by \ref ldnExit if needed.
 */
Result ldnCloseStation(void);

/**
 * @brief Connect
 * @note \ref LdnState must be ::LdnState_Station, this eventually sets the State to ::LdnState_StationConnected.
 * @note This is identical to \ref ldnConnectPrivate besides the used params, the code overwriting LdnSecurityConfig::type also differs.
 * @param[in] sec_config \ref LdnSecurityConfig
 * @param[in] user_config \ref LdnUserConfig
 * @param[in] version LocalCommunicationVersion, this must be 0x0-0x7FFF.
 * @param[in] option ConnectOption bitmask, must be <=0x1. You can use value 0 for example here.
 * @param[in] network_info \ref LdnNetworkInfo
 */
Result ldnConnect(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, s32 version, u32 option, const LdnNetworkInfo *network_info);

/**
 * @brief ConnectPrivate
 * @note \ref LdnState must be ::LdnState_Station, this eventually sets the State to ::LdnState_StationConnected.
 * @note See \ref ldnConnect.
 * @param[in] sec_config \ref LdnSecurityConfig
 * @param[in] sec_param \ref LdnSecurityParameter
 * @param[in] user_config \ref LdnUserConfig
 * @param[in] version LocalCommunicationVersion, this must be 0x0-0x7FFF.
 * @param[in] option ConnectOption bitmask, must be <=0x1. You can use value 0 for example here.
 * @param[in] network_config \ref LdnNetworkConfig
 */
Result ldnConnectPrivate(const LdnSecurityConfig *sec_config, const LdnSecurityParameter *sec_param, const LdnUserConfig *user_config, s32 version, u32 option, const LdnNetworkConfig *network_config);

/**
 * @brief Disconnect
 * @note \ref LdnState must be ::LdnState_StationConnected, this eventually sets the State to ::LdnState_Station.
 */
Result ldnDisconnect(void);

/**
 * @brief SetOperationMode
 * @note With ::LdnServiceType_System this is only available on [4.0.0+].
 * @note With ::LdnServiceType_User this is only available on [19.0.0+].
 * @note \ref LdnState must be ::LdnState_Initialized.
 * @param[in] mode \ref LdnOperationMode
 */
Result ldnSetOperationMode(LdnOperationMode mode);

/**
 * @brief EnableActionFrame
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_Initialized.
 * @param[in] settings \ref LdnActionFrameSettings
 */
Result ldnEnableActionFrame(const LdnActionFrameSettings *settings);

/**
 * @brief DisableActionFrame
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_Initialized.
 */
Result ldnDisableActionFrame(void);

/**
 * @brief SendActionFrame
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_AccessPointCreated / ::LdnState_Station.
 * @param[in] data Data buffer.
 * @param[in] size Data buffer size.
 * @param[in] destination Destination \ref LdnMacAddress.
 * @param[in] bssid Bssid \ref LdnMacAddress.
 * @param[in] channel Channel, must be non-zero.
 * @param[in] flags MessageFlag bit0 clear = block until the data can be sent, set = return error when the data can't be sent.
 */
Result ldnSendActionFrame(const void* data, size_t size, LdnMacAddress destination, LdnMacAddress bssid, s16 channel, u32 flags);

/**
 * @brief RecvActionFrame
 * @note Only available on [18.0.0+].
 * @note \ref ldnEnableActionFrame must be used prior to this.
 * @param[out] data Output data buffer.
 * @param[in] size Max size of the data buffer.
 * @param[out] addr0 First \ref LdnMacAddress.
 * @param[out] addr1 Second \ref LdnMacAddress.
 * @param[out] channel Channel
 * @param[out] out_size Output size.
 * @param[out] link_level LinkLevel
 * @param[in] flags MessageFlag bit0 clear = block until data is available, set = return error when data is not available.
 */
Result ldnRecvActionFrame(void* data, size_t size, LdnMacAddress *addr0, LdnMacAddress *addr1, s16 *channel, u32 *out_size, s32 *link_level, u32 flags);

/**
 * @brief SetHomeChannel
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_Station.
 * @param[in] channel Channel, must be non-zero.
 */
Result ldnSetHomeChannel(s16 channel);

/**
 * @brief SetTxPower
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_AccessPoint* / ::LdnState_Station*.
 * @param[in] power Power, must be 0x0..0xFF.
 */
Result ldnSetTxPower(s16 power);

/**
 * @brief ResetTxPower
 * @note Only available on [18.0.0+].
 * @note \ref LdnState must be ::LdnState_AccessPoint* / ::LdnState_Station*.
 */
Result ldnResetTxPower(void);

///@}

