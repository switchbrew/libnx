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

typedef enum {
    LdnServiceType_User           = 0, ///< Initializes ldn:u.
    LdnServiceType_System         = 1, ///< Initializes ldn:s.
} LdnServiceType;

/// State loaded by \ref ldnmGetStateForMonitor / \ref ldnGetState.
typedef enum {
    LdnState_None                 = 0, ///< None
    LdnState_Initialized          = 1, ///< Initialized
    LdnState_AccessPointOpened    = 2, ///< AccessPointOpened (\ref ldnOpenAccessPoint)
    LdnState_AccessPointCreated   = 3, ///< AccessPointCreated (\ref ldnCreateNetwork / \ref ldnCreateNetworkPrivate)
    LdnState_StationOpened        = 4, ///< StationOpened (\ref ldnOpenStation)
    LdnState_StationConnected     = 5, ///< StationConnected (\ref ldnConnect / \ref ldnConnectPrivate)
    LdnState_Error                = 6, ///< Error
} LdnState;

/// DisconnectReason loaded by \ref ldnGetDisconnectReason.
typedef enum {
    LdnDisconnectReason_None                     = 0, ///< None
    LdnDisconnectReason_User                     = 1, ///< User
    LdnDisconnectReason_SystemRequest            = 2, ///< SystemRequest
    LdnDisconnectReason_DestroyedByAdmin         = 3, ///< DestroyedByAdmin
    LdnDisconnectReason_DestroyedBySystemRequest = 4, ///< DestroyedBySystemRequest
    LdnDisconnectReason_Admin                    = 5, ///< Admin
    LdnDisconnectReason_SignalLost               = 6, ///< SignalLost
} LdnDisconnectReason;

/// ScanFilterFlags
typedef enum {
    LdnScanFilterFlags_LocalCommunicationId      = BIT(0),  ///< When set, enables using LdnScanFilter::local_communication_id.
    LdnScanFilterFlags_NetworkId                 = BIT(1),  ///< When set, enables using LdnScanFilter::network_id.
    LdnScanFilterFlags_Unknown2                  = BIT(2),  ///< When set, enables using LdnScanFilter::unk_x20.
    LdnScanFilterFlags_MacAddr                   = BIT(3),  ///< When set, enables using LdnScanFilter::mac_addr. Only available with \ref ldnScanPrivate.
    LdnScanFilterFlags_Ssid                      = BIT(4),  ///< When set, enables using the LdnScanFilter::ssid.
    LdnScanFilterFlags_UserData                  = BIT(5),  ///< When set, enables using LdnScanFilter::userdata_filter.
} LdnScanFilterFlags;

/// AcceptPolicy
typedef enum {
    LdnAcceptPolicy_AllowAll       = 0, ///< Allow all.
    LdnAcceptPolicy_DenyAll        = 1, ///< Deny all.
    LdnAcceptPolicy_Blacklist      = 2, ///< Blacklist, addresses in the list (\ref ldnAddAcceptFilterEntry) are not allowed.
    LdnAcceptPolicy_Whitelist      = 3, ///< Whitelist, only addresses in the list (\ref ldnAddAcceptFilterEntry) are allowed.
} LdnAcceptPolicy;

/// OperationMode
typedef enum {
    LdnOperationMode_Unknown0     = 0, ///< Unknown
    LdnOperationMode_Unknown1     = 1, ///< Unknown
} LdnOperationMode;

/// WirelessControllerRestriction
typedef enum {
    LdnWirelessControllerRestriction_Unknown0 = 0,   ///< Unknown
    LdnWirelessControllerRestriction_Unknown1 = 1,   ///< Unknown
} LdnWirelessControllerRestriction;

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
    u8 val;                            ///< The field in state is reset to zero by \ref ldnGetNetworkInfoLatestUpdate after loading it.
    u8 reserved[0x7];                  ///< Not initialized with \ref ldnGetNetworkInfoLatestUpdate.
} LdnNodeLatestUpdate;

/// AddressEntry
typedef struct {
    LdnIpv4Address ip_addr;            ///< \ref LdnIpv4Address
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress
    u8 pad[0x2];                       ///< Padding
} LdnAddressEntry;

/// NodeInfo
typedef struct {
    LdnIpv4Address ip_addr;            ///< \ref LdnIpv4Address
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress
    s8 id;                             ///< ID / index
    u8 is_connected;                   ///< IsConnected flag
    char nickname[0x20];               ///< LdnUserConfig::nickname
    u8 reserved_x2C[0x2];              ///< Reserved
    s16 local_communication_version;   ///< LocalCommunicationVersion
    u8 reserved_x30[0x10];             ///< Reserved
} LdnNodeInfo;

/// UserConfig. The input struct is copied to a tmp struct, which is then used with the cmd.
typedef struct {
    char nickname[0x20];               ///< NUL-terminated string for the user nickname.
    u8 reserved[0x10];                 ///< Cleared to zero for the tmp struct.
} LdnUserConfig;

/// NetworkInfo
typedef struct {
    u64 local_communication_id;        ///< LocalCommunicationId
    u8 reserved_x8[0x2];               ///< Reserved
    u16 userdata_filter;               ///< Arbitrary user data which can be used for filtering with \ref LdnScanFilter.
    u8 reserved_xC[0x4];               ///< Reserved
    u8 network_id[0x10];               ///< LdnSecurityParameter::network_id. NetworkId which is used to generate/overwrite the ssid. With \ref ldnScan / \ref ldnScanPrivate, this is only done after filtering when unk_x4B is value 0x2.
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress
    LdnSsid ssid;                      ///< \ref LdnSsid
    s16 network_channel;               ///< NetworkChannel
    s8 link_level;                     ///< LinkLevel
    u8 unk_x4B;                        ///< Unknown. Set to hard-coded value 0x2 with output structs, except with \ref ldnScan / \ref ldnScanPrivate which can also set value 0x1 in certain cases.
    u8 pad_x4C[0x4];                   ///< Padding
    u8 sec_param_data[0x10];           ///< LdnSecurityParameter::data
    u16 sec_type;                      ///< LdnSecurityConfig::type
    u8 accept_policy;                  ///< \ref LdnAcceptPolicy
    u8 unk_x63;                        ///< Only set with \ref ldnScan / \ref ldnScanPrivate, when unk_x4B is value 0x2.
    u8 pad_x64[0x2];                   ///< Padding
    s8 participant_max;                ///< Maximum participants, for nodes.
    u8 participant_num;                ///< ParticipantNum, number of set entries in nodes. If unk_x4B is not 0x2, ParticipantNum should be handled as if it's 0.
    LdnNodeInfo nodes[8];              ///< Array of \ref LdnNodeInfo, starting with the AccessPoint node.
    u8 reserved_x268[0x2];             ///< Reserved
    u16 advertise_data_size;           ///< AdvertiseData size (\ref ldnSetAdvertiseData)
    u8 advertise_data[0x180];          ///< AdvertiseData (\ref ldnSetAdvertiseData)
    u8 reserved_x3EC[0x8C];            ///< Reserved
    u64 auth_id;                       ///< Random AuthenticationId.
} LdnNetworkInfo;

/// ScanFilter. The input struct is copied to a tmp struct, which is then used with the cmd (\ref ldnScan and \ref ldnScanPrivate).
typedef struct {
    s64 local_communication_id;        ///< See ::LdnScanFilterFlags_LocalCommunicationId. When enabled, this will be overwritten if it's -1 (written data is from the user-process control.nacp, with value 0 used instead if loading fails). During filtering if enabled, LdnNetworkInfo::unk_x4B must match 0x2, and this ScanFilter field must match LdnNetworkInfo::local_communication_id.
    u8 pad_x8[0x2];                    ///< Padding
    u16 userdata_filter;               ///< See ::LdnScanFilterFlags_UserData. During filtering if enabled, LdnNetworkInfo::unk_x4B must match 0x2, and this ScanFilter field must match LdnNetworkInfo::userdata_filter.
    u8 pad_xC[0x4];                    ///< Padding
    u8 network_id[0x10];               ///< See ::LdnScanFilterFlags_NetworkId. During filtering if enabled, LdnNetworkInfo::unk_x4B must match 0x2, and this ScanFilter data must match LdnNetworkInfo::network_id.
    u32 unk_x20;                       ///< See ::LdnScanFilterFlags_Unknown2. When enabled, this must be <=0x3, and during filtering must match LdnNetworkInfo::unk_x4B.
    LdnMacAddress mac_addr;            ///< \ref LdnMacAddress (::LdnScanFilterFlags_MacAddr, during filtering if enabled this must match LdnNetworkInfo::mac_addr)
    LdnSsid ssid;                      ///< \ref LdnSsid (::LdnScanFilterFlags_Ssid, during filtering if enabled this must match LdnNetworkInfo::ssid)
    u8 reserved[0x10];                 ///< Cleared to zero for the tmp struct.
    u32 flags;                         ///< Bitmask for \ref LdnScanFilterFlags. Masked with value 0x37 for \ref ldnScan, with \ref ldnScanPrivate this is masked with 0x3F.
} LdnScanFilter;

/// SecurityConfig
typedef struct {
    u16 type;                          ///< Type, a default of value 0x1 can be used here. Overwritten by \ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnect, \ref ldnConnectPrivate.
    u16 data_size;                     ///< Data size. Must be 0x10-0x40.
    u8 data[0x40];                     ///< Data, used with key derivation.
} LdnSecurityConfig;

/// SecurityParameter. The struct used by \ref ldnCreateNetwork internally is randomly-generated.
typedef struct {
    u8 data[0x10];                     ///< Data, used with the same key derivation as \ref LdnSecurityConfig.
    u8 network_id[0x10];               ///< LdnNetworkInfo::network_id
} LdnSecurityParameter;

/// NetworkConfig. The input struct is copied to a tmp struct, which is then used with the cmd (\ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnectPrivate).
typedef struct {
    s64 local_communication_id;        ///< LdnNetworkInfo::local_communication_id. \ref ldnCreateNetwork, \ref ldnCreateNetworkPrivate, \ref ldnConnect, \ref ldnConnectPrivate: When -1, this is overwritten with the first LocalCommunicationId from the user-process control.nacp, if loading fails value 0 is written instead. Otherwise when not -1, if control.nacp loading is successful, this field must match one of the LocalCommunicationIds from there.
    u8 reserved_x8[2];                 ///< Cleared to zero for the tmp struct.
    u16 userdata_filter;               ///< LdnNetworkInfo::userdata_filter
    u8 reserved_xC[4];                 ///< Cleared to zero for the tmp struct.
    s16 network_channel;               ///< LdnNetworkInfo::network_channel. Channel, can be zero. Overwritten internally by \ref ldnCreateNetwork.
    s8 participant_max;                ///< LdnNetworkInfo::participant_max. \ref ldnCreateNetwork / \ref ldnCreateNetworkPrivate: Must be 0x1-0x8.
    u8 reserved_x13;                   ///< Cleared to zero for the tmp struct.
    s16 local_communication_version;   ///< LdnNodeInfo::local_communication_version, for the first entry in LdnNetworkInfo::nodes. Must not be negative.
    u8 reserved_x16[0xA];              ///< Cleared to zero for the tmp struct.
} LdnNetworkConfig;

///@name ldn:m
///@{

/// Initialize ldn:m.
Result ldnmInitialize(void);

/// Exit ldn:m.
void ldnmExit(void);

/// Gets the Service object for IMonitorService.
Service* ldnmGetServiceSession_MonitorService(void);

/**
 * @brief GetStateForMonitor
 * @param[out] out \ref LdnState
 */
Result ldnmGetStateForMonitor(LdnState *out);

/**
 * @brief GetNetworkInfoForMonitor
 * @param[out] out \ref LdnNetworkInfo
 */
Result ldnmGetNetworkInfoForMonitor(LdnNetworkInfo *out);

/**
 * @brief GetIpv4AddressForMonitor
 * @param[out] addr \ref LdnIpv4Address
 * @param[out] mask \ref LdnSubnetMask
 */
Result ldnmGetIpv4AddressForMonitor(LdnIpv4Address *addr, LdnSubnetMask *mask);

/**
 * @brief GetSecurityParameterForMonitor
  * @note Not exposed by official sw.
 * @param[out] out \ref LdnSecurityParameter
 */
Result ldnmGetSecurityParameterForMonitor(LdnSecurityParameter *out);

/**
 * @brief GetNetworkConfigForMonitor
  * @note Not exposed by official sw.
 * @param[out] out \ref LdnNetworkConfig
 */
Result ldnmGetNetworkConfigForMonitor(LdnNetworkConfig *out);

///@}

///@name ldn
///@{

/// Initialize ldn.
Result ldnInitialize(LdnServiceType service_type);

/// Exit ldn.
void ldnExit(void);

/// Gets the Service object for IUserLocalCommunicationService/ISystemLocalCommunicationService.
Service* ldnGetServiceSession_LocalCommunicationService(void);

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
 * @brief AttachStateChangeEvent
 * @note The Event must be closed by the user once finished with it.
 * @note This is signaled when the data returned by \ref ldnGetNetworkInfo / \ref ldnGetNetworkInfoLatestUpdate is updated.
 * @param[out] out_event Output Event with autoclear=true.
 */
Result ldnAttachStateChangeEvent(Event* out_event);

/**
 * @brief GetNetworkInfoLatestUpdate
 * @param[out] network_info \ref LdnNetworkInfo
 * @param[out] nodes Output array of \ref LdnNodeLatestUpdate.
 * @param[in] count Size of the nodes array in entries, must be 8.
 */
Result ldnGetNetworkInfoLatestUpdate(LdnNetworkInfo *network_info, LdnNodeLatestUpdate *nodes, s32 count);

/**
 * @brief Scan
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, ::LdnState_StationOpened, or ::LdnState_StationConnected.
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
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, ::LdnState_StationOpened, or ::LdnState_StationConnected.
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
 * @brief OpenAccessPoint
 * @note \ref LdnState must be ::LdnState_Initialized, this eventually sets the State to ::LdnState_AccessPointOpened.
 */
Result ldnOpenAccessPoint(void);

/**
 * @brief CloseAccessPoint
 * @note \ref LdnState must be ::LdnState_AccessPointOpened or ::LdnState_AccessPointCreated, this eventually sets the State to ::LdnState_Initialized.
 * @note Used automatically internally by \ref ldnExit if needed.
 */
Result ldnCloseAccessPoint(void);

/**
 * @brief CreateNetwork
 * @note \ref LdnState must be ::LdnState_AccessPointOpened, this eventually sets the State to ::LdnState_AccessPointCreated.
 * @param[in] sec_config \ref LdnSecurityConfig
 * @param[in] user_config \ref LdnUserConfig
 * @param[in] network_config \ref LdnNetworkConfig
 */
Result ldnCreateNetwork(const LdnSecurityConfig *sec_config, const LdnUserConfig *user_config, const LdnNetworkConfig *network_config);

/**
 * @brief CreateNetworkPrivate
 * @note \ref LdnState must be ::LdnState_AccessPointOpened, this eventually sets the State to ::LdnState_AccessPointCreated.
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
 * @note \ref LdnState must be ::LdnState_AccessPointCreated, this eventually sets the State to ::LdnState_AccessPointOpened.
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
 * @note \ref LdnState must be ::LdnState_AccessPointOpened or ::LdnState_AccessPointCreated.
 * @param[in] buffer Input buffer containing arbitrary user data.
 * @param[in] size Input buffer size, must be <=0x180. If this isn't enough space, you can for example also periodically use this cmd with different regions of your data with some sequence_number field (or use sockets while connected to the network).
 */
Result ldnSetAdvertiseData(const void* buffer, size_t size);

/**
 * @brief SetStationAcceptPolicy
 * @note \ref LdnState must be ::LdnState_AccessPointOpened or ::LdnState_AccessPointCreated.
 * @param[in] policy \ref LdnAcceptPolicy
 */
Result ldnSetStationAcceptPolicy(LdnAcceptPolicy policy);

/**
 * @brief AddAcceptFilterEntry
 * @note \ref LdnState must be ::LdnState_AccessPointOpened or ::LdnState_AccessPointCreated.
 * @note See \ref LdnAcceptPolicy.
 * @param[in] addr \ref LdnMacAddress. If you want, you can also pass LdnNodeInfo::mac_addr for this.
 */
Result ldnAddAcceptFilterEntry(LdnMacAddress addr);

/**
 * @brief ClearAcceptFilter
 * @note \ref LdnState must be ::LdnState_AccessPointOpened or ::LdnState_AccessPointCreated.
 */
Result ldnClearAcceptFilter(void);

/**
 * @brief OpenStation
 * @note \ref LdnState must be ::LdnState_Initialized, this eventually sets the State to ::LdnState_StationOpened.
 */
Result ldnOpenStation(void);

/**
 * @brief CloseStation
 * @note \ref LdnState must be ::LdnState_StationOpened or ::LdnState_StationConnected, this eventually sets the State to ::LdnState_Initialized.
 * @note Used automatically internally by \ref ldnExit if needed.
 */
Result ldnCloseStation(void);

/**
 * @brief Connect
 * @note \ref LdnState must be ::LdnState_StationOpened, this eventually sets the State to ::LdnState_StationConnected.
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
 * @note \ref LdnState must be ::LdnState_StationOpened, this eventually sets the State to ::LdnState_StationConnected.
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
 * @note \ref LdnState must be ::LdnState_StationConnected, this eventually sets the State to ::LdnState_StationOpened.
 */
Result ldnDisconnect(void);

/**
 * @brief SetOperationMode
 * @note Only available on [4.0.0+].
 * @note Only available with ::LdnServiceType_System.
 * @note \ref LdnState must be ::LdnState_Initialized.
 * @param[in] mode \ref LdnOperationMode
 */
Result ldnSetOperationMode(LdnOperationMode mode);

///@}

