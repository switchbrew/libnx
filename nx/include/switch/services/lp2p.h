/**
 * @file lp2p.h
 * @brief lp2p service IPC wrapper, for local-WLAN communications with accessories. See also: https://switchbrew.org/wiki/LDN_services
 * @note Only available on [9.1.0+].
 * @author yellows8
 * @copyright libnx Authors
 */

#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../kernel/event.h"

typedef enum {
    Lp2pServiceType_App           = 0, ///< Initializes lp2p:app.
    Lp2pServiceType_System        = 1, ///< Initializes lp2p:sys.
} Lp2pServiceType;

/// MacAddress
typedef struct {
    u8 addr[6];                        ///< Address
} Lp2pMacAddress;

/// GroupId
typedef struct {
    u8 id[0x6];                        ///< BSSID
} Lp2pGroupId;

/// GroupInfo
/// \ref lp2pScan only uses the following fields for the cmd input struct: supported_platform/priority, frequency/channel, and preshared_key_binary_size/preshared_key. 
typedef struct {
    u8 unk_x0[0x10];                   ///< When zero, this is set to randomly-generated data. Used during key derivation.
    u64 local_communication_id;        ///< LocalCommunicationId. When zero, the value from the user-process control.nacp is loaded. This is later validated by \ref lp2pJoin / \ref lp2pCreateGroup the same way as LdnNetworkConfig::local_communication_id. Used during key derivation.
    Lp2pGroupId group_id;              ///< Should be all-zero for the input struct so that the default is used.
    char service_name[0x21];           ///< ServiceName. NUL-terminated string for the SSID. These characters must be '-' or alphanumeric (lowercase/uppercase). '_' must not be used, unless you generate valid data for that. The data for '_' will be automatically generated if it's not present.
    s8 flags_count;                    ///< Must be <=0x3F.
    s8 flags[0x40];                    ///< Array of s8 with the above count. Each entry value must be <=0x3F. Each entry is an array index used to load a set of flags from a global array with the specified index.
    u8 supported_platform;             ///< SupportedPlatform. Must match value 1. 0 is PlatformIdNX, 1 is PlatformIdFuji.
    s8 member_count_max;               ///< MemberCountMax. Must be <=0x8. If zero during group-creation, a default of value 1 is used for the value passed to a service-cmd.
    u8 unk_x82;                        ///< Unknown
    u8 unk_x83;                        ///< Unknown
    u16 frequency;                     ///< Wifi frequency: 24 = 2.4GHz, 50 = 5GHz.
    s16 channel;                       ///< Wifi channel number. 0 = use default, otherwise this must be one of the following depending on the frequency field. 24: 1, 6, 11. 50: 36, 40, 44, 48.
    u8 network_mode;                   ///< NetworkMode
    u8 performance_requirement;        ///< PerformanceRequirement
    u8 security_type;                  ///< Security type, used during key derivation. 0 = use defaults, 1 = plaintext, 2 = encrypted. [11.0.0+] 3: Standard WPA2-PSK.
    s8 static_aes_key_index;           ///< StaticAesKeyIndex. Used as the array-index for selecting the KeySource used with GenerateAesKek during key derivation. Should be 1-2, otherwise GenerateAesKek is skipped and zeros are used for the AccessKey instead.
    u8 unk_x8C;                        ///< Unknown
    u8 priority;                       ///< Priority. Must match one of the following, depending on the used service (doesn't apply to \ref lp2pJoin): 55 = SystemPriority (lp2p:sys), 90 = ApplicationPriority (lp2p:app and lp2p:sys).
    u8 stealth_enabled;                ///< StealthEnabled. Bool flag, controls whether the SSID is hidden.
    u8 unk_x8F;                        ///< If zero, a default value of 0x20 is used.
    u8 unk_x90[0x130];                 ///< Unknown
    u8 preshared_key_binary_size;      ///< PresharedKeyBinarySize
    u8 preshared_key[0x3F];            ///< PresharedKey. Used during key derivation.
} Lp2pGroupInfo;

/// ScanResult
typedef struct {
    Lp2pGroupInfo group_info;          ///< \ref Lp2pGroupInfo
    u8 unk_x200;                       ///< Unknown
    u8 unk_x201[0x5];                  ///< Unknown
    u16 advertise_data_size;           ///< Size of the following AdvertiseData.
    u8 advertise_data[0x80];           ///< AdvertiseData, with the above size. This originates from \ref lp2pSetAdvertiseData.
    u8 unk_x288[0x78];                 ///< Unknown
} Lp2pScanResult;

/// NodeInfo
typedef struct {
    u8 ip_addr[0x20];                  ///< struct sockaddr for the IP address.
    u8 unk_x20[0x4];                   ///< Unknown
    Lp2pMacAddress mac_addr;           ///< \ref Lp2pMacAddress
    u8 unk_x2A[0x56];                  ///< Unknown
} Lp2pNodeInfo;

/// IpConfig. Only contains IPv4 addresses.
typedef struct {
    u8 unk_x0[0x20];                   ///< Always zeros.
    u8 ip_addr[0x20];                  ///< struct sockaddr for the IP address.
    u8 subnet_mask[0x20];              ///< struct sockaddr for the subnet-mask.
    u8 gateway[0x20];                  ///< struct sockaddr for the gateway(?).
    u8 unk_x80[0x80];                  ///< Always zeros.
} Lp2pIpConfig;

/// Initialize lp2p.
Result lp2pInitialize(Lp2pServiceType service_type);

/// Exit lp2p.
void lp2pExit(void);

/// Gets the Service object for INetworkService.
Service* lp2pGetServiceSession_INetworkService(void);

/// Gets the Service object for INetworkServiceMonitor.
Service* lp2pGetServiceSession_INetworkServiceMonitor(void);

/**
 * @brief Creates a default \ref Lp2pGroupInfo for use with \ref lp2pCreateGroup / \ref lp2pJoin.
 * @param info \ref Lp2pGroupInfo
 */
void lp2pCreateGroupInfo(Lp2pGroupInfo *info);

/**
 * @brief Creates a default \ref Lp2pGroupInfo for use with \ref lp2pScan.
 * @param info \ref Lp2pGroupInfo
 */
void lp2pCreateGroupInfoScan(Lp2pGroupInfo *info);

/**
 * @brief Sets Lp2pGroupInfo::service_name.
 * @param info \ref Lp2pGroupInfo
 * @param[in] name ServiceName / SSID.
 */
void lp2pGroupInfoSetServiceName(Lp2pGroupInfo *info, const char *name);

/**
 * @brief Sets Lp2pGroupInfo::flags_count and Lp2pGroupInfo::flags.
 * @note The default is count=1 flags[0]=1, which is used by \ref lp2pCreateGroupInfo. [11.0.0+] To use standard WPA2-PSK, you can use flags[0]=0.
 * @param info \ref Lp2pGroupInfo
 * @param[in] flags Lp2pGroupInfo::flags
 * @param[in] count Lp2pGroupInfo::flags_count
 */
void lp2pGroupInfoSetFlags(Lp2pGroupInfo *info, s8 *flags, size_t count);

/**
 * @brief Sets Lp2pGroupInfo::member_count_max.
 * @param info \ref Lp2pGroupInfo
 * @param[in] count MemberCountMax
 */
NX_CONSTEXPR void lp2pGroupInfoSetMemberCountMax(Lp2pGroupInfo *info, size_t count) {
    info->member_count_max = count;
}

/**
 * @brief Sets Lp2pGroupInfo::frequency and Lp2pGroupInfo::channel.
 * @param info \ref Lp2pGroupInfo
 * @param[in] frequency Lp2pGroupInfo::frequency
 * @param[in] channel Lp2pGroupInfo::channel
 */
NX_CONSTEXPR void lp2pGroupInfoSetFrequencyChannel(Lp2pGroupInfo *info, u16 frequency, s16 channel) {
    info->frequency = frequency;
    info->channel = channel;
}

/**
 * @brief Sets Lp2pGroupInfo::stealth_enabled.
 * @param info \ref Lp2pGroupInfo
 * @param[in] flag Lp2pGroupInfo::stealth_enabled
 */
NX_CONSTEXPR void lp2pGroupInfoSetStealthEnabled(Lp2pGroupInfo *info, bool flag) {
    info->stealth_enabled = flag!=0;
}

/**
 * @brief Sets the PresharedKey for the specified \ref Lp2pGroupInfo.
 * @note Using this is required before using the \ref Lp2pGroupInfo as input for any cmds, so that Lp2pGroupInfo::preshared_key_binary_size gets initialized.
 * @note If standard WPA2-PSK is being used, use \ref lp2pGroupInfoSetPassphrase instead.
 * @param info \ref Lp2pGroupInfo
 * @param[in] key Data for the PresharedKey.
 * @param[in] size Size to copy into the PresharedKey, max is 0x20.
 */
void lp2pGroupInfoSetPresharedKey(Lp2pGroupInfo *info, const void* key, size_t size);

/**
 * @brief Sets the passphrase, for when standard WPA2-PSK is being used.
 * @note Configure standard WPA2-PSK usage via \ref lp2pGroupInfoSetFlags / Lp2pGroupInfo::security_type.
 * @note Only available on [11.0.0+].
 * @param info \ref Lp2pGroupInfo
 * @param[in] passphrase Passphrase string, the required length is 0x8-0x3F.
 */
Result lp2pGroupInfoSetPassphrase(Lp2pGroupInfo *info, const char *passphrase);

///@name INetworkService
///@{

/**
 * @brief Scan
 * @param[in] info \ref Lp2pGroupInfo
 * @param[out] results Output array of \ref Lp2pScanResult.
 * @param[in] count Size of the results array in entries.
 * @param[out] total_out Total output entries.
 */
Result lp2pScan(const Lp2pGroupInfo *info, Lp2pScanResult *results, s32 count, s32 *total_out);

/**
 * @brief CreateGroup
 * @note The role (\ref lp2pGetRole) must be 0. This eventually sets the role to value 1.
 * @param[in] info \ref Lp2pGroupInfo
 */
Result lp2pCreateGroup(const Lp2pGroupInfo *info);

/**
 * @brief This destroys the previously created group from \ref lp2pCreateGroup.
 * @note If no group was previously created (role from \ref lp2pGetRole is not 1), this just returns 0.
 */
Result lp2pDestroyGroup(void);

/**
 * @brief SetAdvertiseData
 * @note The role (\ref lp2pGetRole) must be <=1.
 * @note An empty buffer (buffer=NULL/size=0) can be used to reset the AdvertiseData size in state to zero.
 * @param[out] buffer Input buffer containing arbitrary user data.
 * @param[in] size Input buffer size, must be <=0x80.
 */
Result lp2pSetAdvertiseData(const void* buffer, size_t size);

/**
 * @brief This sends an Action frame to the specified \ref Lp2pGroupId, with the specified destination \ref Lp2pMacAddress.
 * @note The role (\ref lp2pGetRole) must be non-zero.
 * @note The error from \ref lp2pGetNetworkInterfaceLastError will be returned if it's set.
 * @note [11.0.0+] Lp2pGroupInfo::security_type must be value 2 (default encryption), otherwise an error is returned.
 * @param[in] buffer Input buffer containing arbitrary user data.
 * @param[in] size Input buffer size, must be <=0x400.
 * @param[in] addr \ref Lp2pMacAddress, this can be a broadcast address. This must be non-zero.
 * @param[in] group_id \ref Lp2pGroupId
 * @param[in] frequency Must be >=1. See Lp2pGroupInfo::frequency.
 * @param[in] channel Must be >=1. See Lp2pGroupInfo::channel.
 * @param[in] flags Only bit0 is used: clear = block until the data can be sent, set = return error when the data can't be sent.
 */
Result lp2pSendToOtherGroup(const void* buffer, size_t size, Lp2pMacAddress addr, Lp2pGroupId group_id, s16 frequency, s16 channel, u32 flags);

/**
 * @brief This receives an Action frame.
 * @note The role (\ref lp2pGetRole) must be non-zero.
 * @note When data is not available, the error from \ref lp2pGetNetworkInterfaceLastError will be returned if it's set.
 * @param[out] buffer Output buffer containing arbitrary user data.
 * @param[in] size Output buffer size.
 * @param[in] flags Only bit0 is used: clear = block until data is available, set = return error when data is not available.
 * @param[in] addr \ref Lp2pMacAddress
 * @param[in] unk0 Unknown
 * @param[in] unk1 Unknown
 * @param[out] out_size This is the original size used for copying to the output buffer, before it's clamped to the output-buffer size.
 * @param[out] unk2 Unknown
 */
Result lp2pRecvFromOtherGroup(void* buffer, size_t size, u32 flags, Lp2pMacAddress *addr, u16 *unk0, s32 *unk1, u64 *out_size, s32 *unk2);

/**
 * @brief AddAcceptableGroupId
 * @param[in] group_id \ref Lp2pGroupId
 */
Result lp2pAddAcceptableGroupId(Lp2pGroupId group_id);

/**
 * @brief RemoveAcceptableGroupId
 */
Result lp2pRemoveAcceptableGroupId(void);

///@name INetworkServiceMonitor
///@{

/**
 * @brief AttachNetworkInterfaceStateChangeEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result lp2pAttachNetworkInterfaceStateChangeEvent(Event* out_event);

/**
 * @brief GetNetworkInterfaceLastError
 */
Result lp2pGetNetworkInterfaceLastError(void);

/**
 * @brief GetRole
 * @param[out] out Output Role.
 */
Result lp2pGetRole(u8 *out);

/**
 * @brief GetAdvertiseData
 * @note The role from \ref lp2pGetRole must be value 2.
 * @param[out] buffer Output buffer data.
 * @param[in] size Output buffer size.
 * @param[out] transfer_size Size of the data copied into the buffer.
 * @param[out] original_size Original size from state.
 */
Result lp2pGetAdvertiseData(void* buffer, size_t size, u16 *transfer_size, u16 *original_size);

/**
 * @brief GetAdvertiseData2
 * @note This is identical to \ref lp2pGetAdvertiseData except this doesn't run the role validation.
 * @param[out] buffer Output buffer data.
 * @param[in] size Output buffer size.
 * @param[out] transfer_size Size of the data copied into the buffer.
 * @param[out] original_size Original size from state.
 */
Result lp2pGetAdvertiseData2(void* buffer, size_t size, u16 *transfer_size, u16 *original_size);

/**
 * @brief GetGroupInfo
 * @note The role from \ref lp2pGetRole must be non-zero.
 * @param[out] out \ref Lp2pGroupInfo
 */
Result lp2pGetGroupInfo(Lp2pGroupInfo *out);

/**
 * @brief This runs the same code as \ref lp2pCreateGroup to generate the \ref Lp2pGroupInfo for the input struct.
 * @param[out] out \ref Lp2pGroupInfo
 * @param[in] info \ref Lp2pGroupInfo
 */
Result lp2pJoin(Lp2pGroupInfo *out, const Lp2pGroupInfo *info);

/**
 * @brief GetGroupOwner
 * @note The role from \ref lp2pGetRole must be non-zero.
 * @param[out] out \ref Lp2pNodeInfo
 */
Result lp2pGetGroupOwner(Lp2pNodeInfo *out);

/**
 * @brief GetIpConfig
 * @note The role from \ref lp2pGetRole must be non-zero.
 * @param[out] out \ref Lp2pIpConfig
 */
Result lp2pGetIpConfig(Lp2pIpConfig *out);

/**
 * @brief Leave
 * @param[out] out Output value.
 */
Result lp2pLeave(u32 *out);

/**
 * @brief AttachJoinEvent
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
 */
Result lp2pAttachJoinEvent(Event* out_event);

/**
 * @brief GetMembers
 * @note The role from \ref lp2pGetRole must be value 1.
 * @param[out] members Output array of \ref Lp2pNodeInfo.
 * @param[in] count Size of the members array in entries. A maximum of 8 entries can be returned.
 * @param[out] total_out Total output entries.
 */
Result lp2pGetMembers(Lp2pNodeInfo *members, s32 count, s32 *total_out);

///@}

