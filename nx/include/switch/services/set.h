/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/time.h"
#include "../services/acc.h"
#include "../services/fs.h"
#include "../services/btdrv_types.h"
#include "../services/btm_types.h"
#include "../sf/service.h"

#define SET_MAX_NAME_SIZE 0x48

typedef enum {
    ColorSetId_Light = 0,
    ColorSetId_Dark  = 1,
} ColorSetId;

/// Console Product Models
typedef enum {
    SetSysProductModel_Invalid = 0, ///< Invalid Model
    SetSysProductModel_Nx      = 1, ///< Erista Model
    SetSysProductModel_Copper  = 2, ///< Erista "Simulation" Model
    SetSysProductModel_Iowa    = 3, ///< Mariko Model
    SetSysProductModel_Hoag    = 4, ///< Mariko Lite Model
    SetSysProductModel_Calcio  = 5, ///< Mariko "Simulation" Model
    SetSysProductModel_Aula    = 6, ///< Mariko OLED Model
} SetSysProductModel;

/// IDs for Language.
typedef enum
{
    SetLanguage_JA =    0,   ///< Japanese
    SetLanguage_ENUS =  1,   ///< US English ("AmericanEnglish")
    SetLanguage_FR =    2,   ///< French
    SetLanguage_DE =    3,   ///< German
    SetLanguage_IT =    4,   ///< Italian
    SetLanguage_ES =    5,   ///< Spanish
    SetLanguage_ZHCN =  6,   ///< Simplified Chinese ("Chinese")
    SetLanguage_KO =    7,   ///< Korean
    SetLanguage_NL =    8,   ///< Dutch
    SetLanguage_PT =    9,   ///< Portuguese
    SetLanguage_RU =    10,  ///< Russian
    SetLanguage_ZHTW =  11,  ///< Traditional Chinese ("Taiwanese")
    SetLanguage_ENGB =  12,  ///< GB English ("BritishEnglish")
    SetLanguage_FRCA =  13,  ///< CA French ("CanadianFrench")
    SetLanguage_ES419 = 14,  ///< "LatinAmericanSpanish"
    SetLanguage_ZHHANS = 15, ///< [4.0.0+] ChineseSimplified
    SetLanguage_ZHHANT = 16, ///< [4.0.0+] ChineseTraditional
    SetLanguage_PTBR   = 17, ///< [10.1.0+] "BrazilianPortuguese"
    SetLanguage_Total,       ///< Total languages supported by this enum.
} SetLanguage;

/// Region codes.
typedef enum {
    SetRegion_JPN = 0, ///< Japan
    SetRegion_USA = 1, ///< The Americas
    SetRegion_EUR = 2, ///< Europe
    SetRegion_AUS = 3, ///< Australia/New Zealand
    SetRegion_HTK = 4, ///< Hong Kong/Taiwan/Korea
    SetRegion_CHN = 5, ///< China
} SetRegion;

/// ConnectionFlag
typedef enum {
    SetSysConnectionFlag_ConnectAutomaticallyFlag   = BIT(0),
    SetSysConnectionFlag_Unknown                    = BIT(1),
} SetSysConnectionFlag;

/// AccessPointSecurityType
typedef enum {
    SetSysAccessPointSecurityType_None      = 0,
    SetSysAccessPointSecurityType_Shared    = 1,
    SetSysAccessPointSecurityType_Wpa       = 2,
    SetSysAccessPointSecurityType_Wpa2      = 3,
} SetSysAccessPointSecurityType;

/// AccessPointSecurityStandard
typedef enum {
    SetSysAccessPointSecurityStandard_None  = 0,
    SetSysAccessPointSecurityStandard_Wep   = 1,
    SetSysAccessPointSecurityStandard_Wpa   = 2,
} SetSysAccessPointSecurityStandard;

/// AutoSettings
typedef enum {
    SetSysAutoSettings_AutoIp   = BIT(0),
    SetSysAutoSettings_AutoDns  = BIT(1),
} SetSysAutoSettings;

/// ProxyFlags
typedef enum {
    SetSysProxyFlags_UseProxyFlag               = BIT(0),
    SetSysProxyFlags_ProxyAutoAuthenticateFlag  = BIT(1),
} SetSysProxyFlags;

/// UserSelectorFlag
typedef enum {
    SetSysUserSelectorFlag_SkipsIfSingleUser      = BIT(0),
} SetSysUserSelectorFlag;

/// EulaVersionClockType
typedef enum {
    SetSysEulaVersionClockType_NetworkSystemClock = 0,
    SetSysEulaVersionClockType_SteadyClock        = 1,
} SetSysEulaVersionClockType;

/// NotificationVolume
typedef enum {
    SetSysNotificationVolume_Mute = 0,
    SetSysNotificationVolume_Low  = 1,
    SetSysNotificationVolume_High = 2,
} SetSysNotificationVolume;

/// FriendPresenceOverlayPermission
typedef enum {
    SetSysFriendPresenceOverlayPermission_NotConfirmed    = 0,
    SetSysFriendPresenceOverlayPermission_NoDisplay       = 1,
    SetSysFriendPresenceOverlayPermission_FavoriteFriends = 2,
    SetSysFriendPresenceOverlayPermission_Friends         = 3,
} SetSysFriendPresenceOverlayPermission;

/// AudioDevice
typedef enum {
    SetSysAudioDevice_Console   = 0,
    SetSysAudioDevice_Headphone = 1,
    SetSysAudioDevice_Tv        = 2,
} SetSysAudioDevice;

/// PrimaryAlbumStorage
typedef enum {
    SetSysPrimaryAlbumStorage_Nand                = 0,
    SetSysPrimaryAlbumStorage_SdCard              = 1,
} SetSysPrimaryAlbumStorage;

/// HandheldSleepPlan
typedef enum {
    SetSysHandheldSleepPlan_1Min  = 0,
    SetSysHandheldSleepPlan_3Min  = 1,
    SetSysHandheldSleepPlan_5Min  = 2,
    SetSysHandheldSleepPlan_10Min = 3,
    SetSysHandheldSleepPlan_30Min = 4,
    SetSysHandheldSleepPlan_Never = 5,
} SetSysHandheldSleepPlan;

/// ConsoleSleepPlan
typedef enum {
    SetSysConsoleSleepPlan_1Hour  = 0,
    SetSysConsoleSleepPlan_2Hour  = 1,
    SetSysConsoleSleepPlan_3Hour  = 2,
    SetSysConsoleSleepPlan_6Hour  = 3,
    SetSysConsoleSleepPlan_12Hour = 4,
    SetSysConsoleSleepPlan_Never  = 5,
} SetSysConsoleSleepPlan;

/// AudioOutputModeTarget
typedef enum {
    SetSysAudioOutputModeTarget_Unknown0 = 0,
    SetSysAudioOutputModeTarget_Unknown1 = 1,
    SetSysAudioOutputModeTarget_Unknown2 = 2,
    SetSysAudioOutputModeTarget_Unknown3 = 3,
} SetSysAudioOutputModeTarget;

/// AudioOutputMode
typedef enum {
    SetSysAudioOutputMode_Unknown1 = 1      ///< Default value.
} SetSysAudioOutputMode;

/// ServiceDiscoveryControlSettings
typedef enum {
    SetSysServiceDiscoveryControlSettings_IsChangeEnvironmentIdentifierDisabled = BIT(0),
} SetSysServiceDiscoveryControlSettings;

/// ErrorReportSharePermission
typedef enum {
    SetSysErrorReportSharePermission_NotConfirmed = 0,
    SetSysErrorReportSharePermission_Granted      = 1,
    SetSysErrorReportSharePermission_Denied       = 2,
} SetSysErrorReportSharePermission;

/// KeyboardLayout
typedef enum {
    SetKeyboardLayout_Japanese                    = 0,
    SetKeyboardLayout_EnglishUs                   = 1,
    SetKeyboardLayout_EnglishUsInternational      = 2,
    SetKeyboardLayout_EnglishUk                   = 3,
    SetKeyboardLayout_French                      = 4,
    SetKeyboardLayout_FrenchCa                    = 5,
    SetKeyboardLayout_Spanish                     = 6,
    SetKeyboardLayout_SpanishLatin                = 7,
    SetKeyboardLayout_German                      = 8,
    SetKeyboardLayout_Italian                     = 9,
    SetKeyboardLayout_Portuguese                  = 10,
    SetKeyboardLayout_Russian                     = 11,
    SetKeyboardLayout_Korean                      = 12,
    SetKeyboardLayout_ChineseSimplified           = 13,
    SetKeyboardLayout_ChineseTraditional          = 14,
} SetKeyboardLayout;

/// ChineseTraditionalInputMethod
typedef enum {
    SetChineseTraditionalInputMethod_Unknown1 = 1,
    SetChineseTraditionalInputMethod_Unknown2 = 2,
} SetChineseTraditionalInputMethod;

/// PtmCycleCountReliability
typedef enum {
    PtmCycleCountReliability_Default    = 0,
    PtmCycleCountReliability_Unk        = 1,
} SetSysPtmCycleCountReliability;

/// PlatformRegion. Other values not listed here should be handled as "Unknown".
typedef enum {
    SetSysPlatformRegion_Global = 1,
    SetSysPlatformRegion_China  = 2,
} SetSysPlatformRegion;

/// TouchScreenMode, for "Touch-Screen Sensitivity".
typedef enum {
    SetSysTouchScreenMode_Stylus   = 0,   ///< Stylus.
    SetSysTouchScreenMode_Standard = 1,   ///< Standard, the default.
} SetSysTouchScreenMode;

/// BlockType
typedef enum {
    SetSysBlockType_Audio           = 1,
    SetSysBlockType_Video           = 2,
    SetSysBlockType_VendorSpecific  = 3,
    SetSysBlockType_Speaker         = 4,
} SetSysBlockType;

/// ControllerType
typedef enum {
    SetSysControllerType_JoyConR    = 1,
    SetSysControllerType_JoyConL    = 2,
    SetSysControllerType_ProCon     = 3,
} SetSysControllerType;

/// BatteryLot
typedef struct {
    char lot[0x18];    ///< BatteryLot string.
} SetBatteryLot;

/// NetworkSettings
typedef struct {
    char name[0x40];
    Uuid uuid;
    u32 connection_flags;               ///< Bitmask with \ref SetSysConnectionFlag.
    u32 wired_flag;
    u32 connect_to_hidden_network;      ///< Bitmask with UseStealthNetworkFlag.
    char access_point_ssid[0x20];
    u32 access_point_ssid_len;
    u32 access_point_security_type;     ///< Bitmask with \ref SetSysAccessPointSecurityType.
    u32 access_point_security_standard; ///< Bitmask with \ref SetSysAccessPointSecurityStandard.
    char access_point_passphrase[0x40];
    u32 access_point_passphrase_len;
    u32 auto_settings;                  ///< Bitmask with \ref SetSysAutoSettings.
    u32 manual_ip_address;
    u32 manual_subnet_mask;
    u32 manual_gateway;
    u32 primary_dns;
    u32 secondary_dns;
    u32 proxy_flags;                    ///< Bitmask with \ref SetSysProxyFlags.
    char proxy_server[0x80];
    u16 proxy_port;
    u16 padding1;
    char proxy_autoauth_user[0x20];
    char proxy_autoauth_pass[0x20];
    u16 mtu;
    u16 padding2;
} SetSysNetworkSettings;

/// LcdBacklightBrightnessMapping
typedef struct {
    float brightness_applied_to_backlight;
    float ambient_light_sensor_value;
    float unk_x8;
} SetSysLcdBacklightBrightnessMapping;

/// BacklightSettings
typedef struct {
    u32 auto_brightness_flags;
    float screen_brightness;
    SetSysLcdBacklightBrightnessMapping brightness_mapping;
    float unk_x14;
    float unk_x18;
    float unk_x1C;
    float unk_x20;
    float unk_x24;
} SetSysBacklightSettings;

/// BacklightSettingsEx
typedef struct {
    u32 auto_brightness_flags;
    float screen_brightness;
    float current_brightness_for_vr_mode;
    SetSysLcdBacklightBrightnessMapping brightness_mapping;
    float unk_x18;
    float unk_x1C;
    float unk_x20;
    float unk_x24;
    float unk_x28;
} SetSysBacklightSettingsEx;

/// BluetoothDevicesSettings
typedef struct {
    BtdrvAddress addr;                    ///< \ref BtdrvAddress
    BtmBdName name;                       ///< [1.0.0-12.1.0] BdName. On 13.0.0+ name2 is used instead.
    BtmClassOfDevice class_of_device;     ///< ClassOfDevice
    u8 link_key[0x10];                    ///< LinkKey
    u8 link_key_present;                  ///< LinkKeyPresent
    u16 version;                          ///< Version
    u32 trusted_services;                 ///< TrustedServices
    u16 vid;                              ///< Vid
    u16 pid;                              ///< Pid
    u8 sub_class;                         ///< SubClass
    u8 attribute_mask;                    ///< AttributeMask
    u16 descriptor_length;                ///< DescriptorLength
    u8 descriptor[0x80];                  ///< Descriptor
    u8 key_type;                          ///< KeyType
    u8 device_type;                       ///< DeviceType
    u16 brr_size;                         ///< BrrSize
    u8 brr[0x9];                          ///< Brr
    u8 audio_source_volume;               ///< [13.0.0+] AudioSourceVolume
    char name2[0xF9];                     ///< [13.0.0+] Name
    u8 audio_sink_volume;                 ///< [15.0.0+] AudioSinkVolume
    u32 audio_flags;                      ///< [14.0.0+] AudioFlags
    u8 reserved[0x2C];                    ///< Reserved
} SetSysBluetoothDevicesSettings;

/// Structure returned by \ref setsysGetFirmwareVersion.
typedef struct {
    u8 major;
    u8 minor;
    u8 micro;
    u8 padding1;
    u8 revision_major;
    u8 revision_minor;
    u8 padding2;
    u8 padding3;
    char platform[0x20];
    char version_hash[0x40];
    char display_version[0x18];
    char display_title[0x80];
} SetSysFirmwareVersion;

/// Structure returned by \ref setsysGetFirmwareVersionDigest.
typedef struct {
    char digest[0x40];
} SetSysFirmwareVersionDigest;

/// Structure returned by \ref setsysGetSerialNumber.
typedef struct {
    char number[0x18];
} SetSysSerialNumber;

/// DeviceNickName
typedef struct {
    char nickname[0x80];
} SetSysDeviceNickName;

/// UserSelectorSettings
typedef struct {
    u32 flags;                              ///< Bitmask with \ref SetSysUserSelectorFlag.
} SetSysUserSelectorSettings;

/// AccountSettings
typedef struct {
    SetSysUserSelectorSettings settings;
} SetSysAccountSettings;

typedef struct {
    u32 unk_x0;             ///< 0 for Console and Tv, 2 for Headphones.
    u8 volume;              ///< From 0-15.
} SetSysAudioVolume;

/// EulaVersion
typedef struct {
    u32 version;
    s32 region_code;
    s32 clock_type;                                 ///< \ref SetSysEulaVersionClockType
    u32 pad;
    u64 network_clock_time;                         ///< POSIX timestamp.
    TimeSteadyClockTimePoint steady_clock_time;     ///< \ref TimeSteadyClockTimePoint
} SetSysEulaVersion;

/// NotificationTime
typedef struct {
    s32 hour;
    s32 minute;
} SetSysNotificationTime;

/// NotificationSettings
typedef struct {
    u32 flags;                                 ///< Bitmask with NotificationFlag.
    s32 volume;                                ///< \ref SetSysNotificationVolume
    SetSysNotificationTime start_time;         ///< \ref SetSysNotificationTime
    SetSysNotificationTime end_time;           ///< \ref SetSysNotificationTime
} SetSysNotificationSettings;

/// AccountNotificationSettings
typedef struct {
    AccountUid uid;                            ///< \ref AccountUid
    u32 flags;                                 ///< Bitmask with AccountNotificationFlag.
    s8 friend_presence_overlay_permission;     ///< \ref SetSysFriendPresenceOverlayPermission
    u8 pad[3];                                 ///< Padding.
} SetSysAccountNotificationSettings;

/// TvSettings
typedef struct {
    u32 flags;                   ///< Bitmask with TvFlag.
    s32 tv_resolution;           ///< \ref SetSysTvResolution
    s32 hdmi_content_type;       ///< \ref SetSysHdmiContentType
    s32 rgb_range;               ///< \ref SetSysRgbRange
    s32 cmu_mode;                ///< \ref SetSysCmuMode
    u32 underscan;               ///< Underscan.
    float gamma;                 ///< Gamma.
    float contrast;              ///< Contrast.
} SetSysTvSettings;

typedef struct {
    u16 pixel_clock;                                ///< In 10 kHz units.
    u8 horizontal_active_pixels_lsb;
    u8 horizontal_blanking_pixels_lsb;
    u8 horizontal_blanking_pixels_msb : 4;
    u8 horizontal_active_pixels_msb : 4;
    u8 vertical_active_lines_lsb;
    u8 vertical_blanking_lines_lsb;
    u8 vertical_blanking_lines_msb : 4;
    u8 vertical_active_lines_msb : 4;
    u8 horizontal_sync_offset_pixels_lsb;
    u8 horizontal_sync_pulse_width_pixels_lsb;
    u8 vertical_sync_pulse_width_lines_lsb : 4;
    u8 vertical_sync_offset_lines_lsb : 4;
    u8 vertical_sync_pulse_width_lines_msb : 2;
    u8 vertical_sync_offset_lines_msb : 2;
    u8 horizontal_sync_pulse_width_pixels_msb : 2;
    u8 horizontal_sync_offset_pixels_msb : 2;
    u8 horizontal_image_size_mm_lsb;
    u8 vertical_image_size_mm_lsb;
    u8 vertical_image_size_mm_msb : 4;
    u8 horizontal_image_size_mm_msb : 4;
    u8 horizontal_border_pixels;
    u8 vertical_border_lines;
    u8 features_bitmap_0 : 1;
    u8 features_bitmap_1 : 1;
    u8 features_bitmap_2 : 1;
    u8 features_bitmap_34 : 2;
    u8 features_bitmap_56 : 2;
    u8 interlaced : 1;
} SetSysModeLine;

typedef struct {
    struct {
        u8 size : 5;
        SetSysBlockType block_type : 3;
        struct {
            u8 svd_index : 7;
            u8 native_flag : 1;
        } svd[0xC];
    } NX_PACKED video;
    struct {
        u8 size : 5;
        SetSysBlockType block_type : 3;
        u8 channel_count : 3;
        u8 format_code : 4;
        u8 padding1 : 1;
        u8 sampling_rates_bitmap;
        u8 bitrate;
    } NX_PACKED audio;
    struct {
        u8 size : 5;
        SetSysBlockType block_type : 3;
        u8 ieee_registration_id[3];
        u16 source_physical_address;
        u8 mode_bitmap;
        u8 max_tmds_frequency;
        u8 latency_bitmap;
    } NX_PACKED vendor_specific;
    u8 padding[2];
} SetSysDataBlock;

/// Edid
typedef struct {
    u8 pattern[8];                          ///< Fixed pattern 00 FF FF FF FF FF FF 00.
    u16 pnp_id;                             ///< Big-endian set of 3 5-bit values representing letters, 1 = A .. 26 = Z.
    u16 product_code;
    u32 serial_number;
    u8 manufacture_week;
    u8 manufacture_year;                    ///< Real value is val - 10.
    u8 edid_version;
    u8 edid_revision;
    u8 video_input_parameters_bitmap;
    u8 display_width;
    u8 display_height;
    u8 display_gamma;
    u8 supported_features_bitmap;
    struct {
        u8 green_y_lsb : 2;
        u8 green_x_lsb : 2;
        u8 red_y_lsb : 2;
        u8 red_x_lsb : 2;
        u8 blue_lsb : 4;
        u8 white_lsb : 4;
        u8 red_x_msb;
        u8 red_y_msb;
        u8 green_x_msb;
        u8 green_y_msb;
        u8 blue_x_msb;
        u8 blue_y_msb;
        u8 white_x_msb;
        u8 white_y_msb;
    } chromaticity;
    u8 timing_bitmap[3];
    struct {
        u8 x_resolution;                    ///< Real value is (val + 31) * 8 pixels.
        u8 vertical_frequency : 6;          ///< Real value is val + 60 Hz.
        u8 aspect_ratio : 2;                ///< 0 = 16:10, 1 = 4:3, 2 = 5:4, 3 = 16:9.
    } timing_info[8];
    SetSysModeLine timing_descriptor[2];
    struct {
        u16 display_descriptor_zero;
        u8 padding1;
        u8 descriptor_type;
        u8 padding2;
        char name[0xD];
    } display_descriptor_name;
    struct {
        u16 display_descriptor_zero;
        u8 padding1;
        u8 descriptor_type;
        u8 range_limit_offsets;
        u8 vertical_field_rate_min;
        u8 vertical_field_rate_max;
        u8 horizontal_line_rate_min;
        u8 horizontal_line_rate_max;
        u8 pixel_clock_rate_max;            ///< Rounded up to multiples of 10 MHz.
        u8 extended_timing_info;
        u8 padding[7];
    } display_descriptor_range_limits;
    u8 extension_count;                     ///< Always 1.
    u8 checksum;                            ///< Sum of all 128 bytes should equal 0 mod 256.
    ///< Extended data.
    u8 extension_tag;                       ///< Always 2 = CEA EDID timing extension.
    u8 revision;
    u8 dtd_start;
    u8 native_dtd_count : 4;
    u8 native_dtd_feature_bitmap : 4;
    SetSysDataBlock data_block;
    SetSysModeLine extended_timing_descriptor[5];
    u8 padding[5];
    u8 extended_checksum;                   ///< Sum of 128 extended bytes should equal 0 mod 256.
    u8 data2[0x80];                         ///< [13.0.0+]
    u8 data3[0x80];                         ///< [13.0.0+]
} SetSysEdid;

/// DataDeletionSettings
typedef struct {
    u32 flags;                   ///< Bitmask with DataDeletionFlag.
    s32 use_count;               ///< Use count.
} SetSysDataDeletionSettings;

/// SleepSettings
typedef struct {
    u32 flags;                   ///< Bitmask with SleepFlag.
    s32 handheld_sleep_plan;     ///< \ref SetSysHandheldSleepPlan
    s32 console_sleep_plan;      ///< \ref SetSysConsoleSleepPlan
} SetSysSleepSettings;

/// InitialLaunchSettings
typedef struct {
    u32 flags;                              ///< Bitmask with InitialLaunchFlag.
    u32 pad;                                ///< Padding.
    TimeSteadyClockTimePoint timestamp;     ///< \ref TimeSteadyClockTimePoint timestamp.
} SetSysInitialLaunchSettings;

/// PtmFuelGaugeParameter
typedef struct {
    u16 rcomp0;
    u16 tempc0;
    u16 fullcap;
    u16 fullcapnom;
    u16 lavgempty;
    u16 qresidual00;
    u16 qresidual10;
    u16 qresidual20;
    u16 qresidual30;
    u16 cycles;         ///< Normally keeps the cycles reg. Unused and contains stack garbage.
    u32 cycles_actual;  ///< Keeps track of cycles. The fuel gauge cycles reg is reset if > 2.00 cycles and added here.
} SetSysPtmFuelGaugeParameter;

/// Actually nn::util::Color4u8Type.
typedef struct {
    u8 field[4];
} SetSysColor4u8Type;

/// NxControllerLegacySettings
typedef struct {
    BtdrvAddress address;
    u8 type;                            ///< \ref SetSysControllerType.
    char serial[0x10];
    SetSysColor4u8Type body_color;
    SetSysColor4u8Type button_color;
    u8 unk_x1F[8];
    u8 unk_x27;
    u8 interface_type;                  ///< Bitmask with \ref XcdInterfaceType.
} SetSysNxControllerLegacySettings;

/// NxControllerSettings
typedef struct {
    BtdrvAddress address;
    u8 type;                            ///< \ref SetSysControllerType.
    char serial[0x10];
    SetSysColor4u8Type body_color;
    SetSysColor4u8Type button_color;
    u8 unk_x1F[8];
    u8 unk_x27;
    u8 interface_type;                  ///< Bitmask with \ref XcdInterfaceType.
    u8 unk_x29[0x403];                  ///< Unknown
} SetSysNxControllerSettings;

/// ConsoleSixAxisSensorAccelerationBias
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
} SetSysConsoleSixAxisSensorAccelerationBias;

/// ConsoleSixAxisSensorAngularVelocityBias
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
} SetSysConsoleSixAxisSensorAngularVelocityBias;

/// ConsoleSixAxisSensorAccelerationGain
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
    float unk_xC;
    float unk_x10;
    float unk_x14;
    float unk_x18;
    float unk_x1C;
    float unk_x20;
} SetSysConsoleSixAxisSensorAccelerationGain;

/// ConsoleSixAxisSensorAngularVelocityGain
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
    float unk_xC;
    float unk_x10;
    float unk_x14;
    float unk_x18;
    float unk_x1C;
    float unk_x20;
} SetSysConsoleSixAxisSensorAngularVelocityGain;

/// AllowedSslHosts
typedef struct {
    u8 hosts[0x100];
} SetSysAllowedSslHosts;

/// HostFsMountPoint
typedef struct {
    char mount[0x100];
} SetSysHostFsMountPoint;

/// BlePairingSettings
typedef struct {
    BtdrvAddress address;
    u16 unk_x6;
    u16 unk_x8;
    u8 unk_xA;
    u8 unk_xB;
    u8 unk_xC;
    u8 unk_xD;
    u8 unk_xE;
    u8 unk_xF;
    u8 padding[0x70];
} SetSysBlePairingSettings;

/// ConsoleSixAxisSensorAngularVelocityTimeBias
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
} SetSysConsoleSixAxisSensorAngularVelocityTimeBias;

/// ConsoleSixAxisSensorAngularAcceleration
typedef struct {
    float unk_x0;
    float unk_x4;
    float unk_x8;
    float unk_xC;
    float unk_x10;
    float unk_x14;
    float unk_x18;
    float unk_x1C;
    float unk_x20;
} SetSysConsoleSixAxisSensorAngularAcceleration;

/// RebootlessSystemUpdateVersion. This is the content of the RebootlessSystemUpdateVersion SystemData, in the "/version" file.
typedef struct {
    u32 version;
    u8 reserved[0x1c];
    char display_version[0x20];
} SetSysRebootlessSystemUpdateVersion;

/// AccountOnlineStorageSettings
typedef struct {
    AccountUid uid;                            ///< \ref AccountUid
    u32 unk_x10;
    u32 unk_x14;
} SetSysAccountOnlineStorageSettings;

/// AnalogStickUserCalibration
typedef struct {
    u16 unk_x0;
    u16 unk_x2;
    u16 unk_x4;
    u16 unk_x6;
    u16 unk_x8;
    u16 unk_xA;
    u16 unk_xC;
    u16 unk_xE;
} SetSysAnalogStickUserCalibration;

/// ThemeId
typedef struct {
    u64 theme_id[0x10];
} SetSysThemeId;

/// ThemeSettings
typedef struct {
    u64 theme_settings;
} SetSysThemeSettings;

/// Output from \ref setsysGetHomeMenuScheme. This contains RGBA8 colors which correspond with the physical shell of the system.
typedef struct {
    u32 main_color;       ///< Main Color.
    u32 back_color;       ///< Back Color.
    u32 sub_color;        ///< Sub Color.
    u32 bezel_color;      ///< Bezel Color.
    u32 extra_color;      ///< Extra Color.
} SetSysHomeMenuScheme;

/// ButtonConfigSettings
typedef struct {
    u8 settings[0x5A8];
} SetSysButtonConfigSettings;

/// ButtonConfigRegisteredSettings
typedef struct {
    u8 settings[0x5C8];
} SetSysButtonConfigRegisteredSettings;

typedef struct {
    u8 offset[0x6];
} SetCalAccelerometerOffset;

typedef struct {
    u8 scale[0x6];
} SetCalAccelerometerScale;

typedef struct {
    u32 size;
    u8 cert[0x70];
} SetCalAmiiboEcdsaCertificate;

typedef struct {
    u32 size;
    u8 cert[0x20];
} SetCalAmiiboEcqvBlsCertificate;

typedef struct {
    u32 size;
    u8 key[0x40];
    u32 generation;
} SetCalAmiiboEcqvBlsKey;

typedef struct {
    u32 size;
    u8 cert[0x90];
} SetCalAmiiboEcqvBlsRootCertificate;

typedef struct {
    u32 size;
    u8 cert[0x14];
} SetCalAmiiboEcqvCertificate;

typedef struct {
    u32 size;
    u8 key[0x50];
    u32 generation;
} SetCalAmiiboKey;

typedef struct {
    u8 calibration[0x9];
} SetCalAnalogStickFactoryCalibration;

typedef struct {
    u8 parameter[0x12];
} SetCalAnalogStickModelParameter;

typedef struct {
    u8 bd_addr[0x6];
} SetCalBdAddress;

typedef struct {
    u8 cfg[0x1E];
} SetCalConfigurationId1;

typedef struct {
    u8 offset[0x6];
} SetCalConsoleSixAxisSensorHorizontalOffset;

typedef struct {
    char code[0x3];          ///< Country code.
} SetCalCountryCode;

typedef struct {
    u8 cert[0x180];
} SetCalEccB233DeviceCertificate;

typedef struct {
    u32 size;
    u8 key[0x50];
    u32 generation;
} SetCalEccB233DeviceKey;

typedef struct {
    u8 cert[0x400];
} SetCalGameCardCertificate;

typedef struct {
    u32 size;               ///< Size of the entire key.
    u8 key[0x130];
    u32 generation;
} SetCalGameCardKey;

typedef struct {
    u8 offset[0x6];
} SetCalGyroscopeOffset;

typedef struct {
    u8 scale[0x6];
} SetCalGyroscopeScale;

typedef struct {
    u8 addr[0x6];           ///< Mac address.
} SetCalMacAddress;

typedef struct {
    u8 cert[0x240];
} SetCalRsa2048DeviceCertificate;

typedef struct {
    u32 size;               ///< Size of the entire key.
    u8 key[0x240];
    u32 generation;
} SetCalRsa2048DeviceKey;

typedef SetSysSerialNumber SetCalSerialNumber;

typedef struct {
    u8 parameter[0x5A];
} SetCalSpeakerParameter;

typedef struct {
    u32 size;               ///< Size of the certificate data.
    u8 cert[0x800];
} SetCalSslCertificate;

typedef struct {
    u32 size;               ///< Size of the entire key.
    u8 key[0x130];
    u32 generation;
} SetCalSslKey;

typedef struct {
    u32 code;               ///< Region code.
} SetCalRegionCode;

/// Initialize set.
Result setInitialize(void);

/// Exit set.
void setExit(void);

/// Gets the Service object for the actual set service session.
Service* setGetServiceSession(void);

/// Converts LanguageCode to \ref SetLanguage.
Result setMakeLanguage(u64 LanguageCode, SetLanguage *Language);

/// Converts \ref SetLanguage to LanguageCode.
Result setMakeLanguageCode(SetLanguage Language, u64 *LanguageCode);

/// Gets the current system LanguageCode.
/// Normally this should be used instead of \ref setGetLanguageCode.
/// LanguageCode is a string, see here: https://switchbrew.org/wiki/Settings_services#LanguageCode
Result setGetSystemLanguage(u64 *LanguageCode);

/// Gets the current LanguageCode, \ref setGetSystemLanguage should be used instead normally.
Result setGetLanguageCode(u64 *LanguageCode);

/// Gets available LanguageCodes.
/// On system-version <4.0.0, max_entries is set to the output from \ref setGetAvailableLanguageCodeCount if max_entries is larger than that.
Result setGetAvailableLanguageCodes(s32 *total_entries, u64 *LanguageCodes, size_t max_entries);

/// Gets total available LanguageCodes.
/// Output total is overridden with value 0 if the total is <0.
Result setGetAvailableLanguageCodeCount(s32 *total);

/// Gets the RegionCode.
Result setGetRegionCode(SetRegion *out);

/**
 * @brief GetQuestFlag
 * @note Only available on [5.0.0+].
 * @param[out] out Output flag.
 */
Result setGetQuestFlag(bool *out);

/**
 * @brief Gets the system's nickname.
 * @note Only available on [10.1.0+].
 * @param[out] nickname \ref SetSysDeviceNickName
 */
Result setGetDeviceNickname(SetSysDeviceNickName *nickname);

/// Initialize setsys.
Result setsysInitialize(void);

/// Exit setsys.
void setsysExit(void);

/// Gets the Service object for the actual setsys service session.
Service* setsysGetServiceSession(void);

/**
 * @brief SetLanguageCode
 * @param[in] LanguageCode LanguageCode.
 */
Result setsysSetLanguageCode(u64 LanguageCode);

/**
 * @brief SetNetworkSettings
 * @param[in] settings Input array of \ref SetSysNetworkSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetNetworkSettings(const SetSysNetworkSettings *settings, s32 count);

/**
 * @brief GetNetworkSettings
 * @param[out] total_out Total output entries.
 * @param[out] versions Output array of \ref SetSysNetworkSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetNetworkSettings(s32 *total_out, SetSysNetworkSettings *settings, s32 count);

/**
 * @brief Gets the system firmware version.
 * @param[out] out Firmware version to populate.
 */
Result setsysGetFirmwareVersion(SetSysFirmwareVersion *out);

/**
 * @brief GetFirmwareVersionDigest
 * @param[out] out \ref SetSysFirmwareVersionDigest
 */
Result setsysGetFirmwareVersionDigest(SetSysFirmwareVersionDigest *out);

/**
 * @brief GetLockScreenFlag
 * @param[out] out Output flag.
 */
Result setsysGetLockScreenFlag(bool *out);

/**
 * @brief SetLockScreenFlag
 * @param[in] flag Input flag.
 */
Result setsysSetLockScreenFlag(bool flag);

/**
 * @brief GetBacklightSettings
 * @param[out] out \ref SetSysBacklightSettings
 */
Result setsysGetBacklightSettings(SetSysBacklightSettings *out);

/**
 * @brief SetBacklightSettings
 * @param[in] settings \ref SetSysBacklightSettings
 */
Result setsysSetBacklightSettings(const SetSysBacklightSettings *settings);

/**
 * @brief SetBluetoothDevicesSettings
 * @param[in] settings Input array of \ref SetSysBluetoothDevicesSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetBluetoothDevicesSettings(const SetSysBluetoothDevicesSettings *settings, s32 count);

/**
 * @brief GetBluetoothDevicesSettings
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysBluetoothDevicesSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetBluetoothDevicesSettings(s32 *total_out, SetSysBluetoothDevicesSettings *settings, s32 count);

/**
 * @brief GetExternalSteadyClockSourceId
 * @param[out] out \ref Uuid
 */
Result setsysGetExternalSteadyClockSourceId(Uuid *out);

/**
 * @brief SetExternalSteadyClockSourceId
 * @param[in] uuid \ref Uuid
 */
Result setsysSetExternalSteadyClockSourceId(const Uuid *uuid);

/**
 * @brief GetUserSystemClockContext
 * @param[out] out \ref TimeSystemClockContext
 */
Result setsysGetUserSystemClockContext(TimeSystemClockContext *out);

/**
 * @brief SetUserSystemClockContext
 * @param[in] context \ref TimeSystemClockContext
 */
Result setsysSetUserSystemClockContext(const TimeSystemClockContext *context);

/**
 * @brief GetAccountSettings
 * @param[out] out \ref SetSysAccountSettings
 */
Result setsysGetAccountSettings(SetSysAccountSettings *out);

/**
 * @brief SetAccountSettings
 * @param[in] settings \ref SetSysAccountSettings
 */
Result setsysSetAccountSettings(SetSysAccountSettings settings);

/**
 * @brief GetAudioVolume
 * @param[in] device \ref SetSysAudioDevice
 * @param[out] out \ref SetSysAudioVolume
 */
Result setsysGetAudioVolume(SetSysAudioDevice device, SetSysAudioVolume *out);

/**
 * @brief SetAudioVolume
 * @param[in] device \ref SetSysAudioDevice
 * @param[in] volume \ref SetSysAudioVolume
 */
Result setsysSetAudioVolume(SetSysAudioDevice device, const SetSysAudioVolume *volume);

/**
 * @brief GetEulaVersions
 * @param[out] total_out Total output entries.
 * @param[out] versions Output array of \ref SetSysEulaVersion.
 * @param[in] count Size of the versions array in entries.
 */
Result setsysGetEulaVersions(s32 *total_out, SetSysEulaVersion *versions, s32 count);

/**
 * @brief SetEulaVersions
 * @param[in] versions Input array of \ref SetSysEulaVersion.
 * @param[in] count Size of the versions array in entries.
 */
Result setsysSetEulaVersions(const SetSysEulaVersion *versions, s32 count);

/// Gets the current system theme.
Result setsysGetColorSetId(ColorSetId *out);

/// Sets the current system theme.
Result setsysSetColorSetId(ColorSetId id);

/**
 * @brief GetConsoleInformationUploadFlag
 * @param[out] out Output flag.
 */
Result setsysGetConsoleInformationUploadFlag(bool *out);

/**
 * @brief SetConsoleInformationUploadFlag
 * @param[in] flag Input flag.
 */
Result setsysSetConsoleInformationUploadFlag(bool flag);

/**
 * @brief GetAutomaticApplicationDownloadFlag
 * @param[out] out Output flag.
 */
Result setsysGetAutomaticApplicationDownloadFlag(bool *out);

/**
 * @brief SetAutomaticApplicationDownloadFlag
 * @param[in] flag Input flag.
 */
Result setsysSetAutomaticApplicationDownloadFlag(bool flag);

/**
 * @brief GetNotificationSettings
 * @param[out] out \ref SetSysNotificationSettings
 */
Result setsysGetNotificationSettings(SetSysNotificationSettings *out);

/**
 * @brief SetNotificationSettings
 * @param[in] settings \ref SetSysNotificationSettings
 */
Result setsysSetNotificationSettings(const SetSysNotificationSettings *settings);

/**
 * @brief GetAccountNotificationSettings
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysAccountNotificationSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetAccountNotificationSettings(s32 *total_out, SetSysAccountNotificationSettings *settings, s32 count);

/**
 * @brief SetAccountNotificationSettings
 * @param[in] settings Input array of \ref SetSysAccountNotificationSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetAccountNotificationSettings(const SetSysAccountNotificationSettings *settings, s32 count);

/**
 * @brief GetVibrationMasterVolume
 * @param[out] out Output volume.
 */
Result setsysGetVibrationMasterVolume(float *out);

/**
 * @brief SetVibrationMasterVolume
 * @param[in] volume Input volume.
 */
Result setsysSetVibrationMasterVolume(float volume);

/**
 * @brief Gets the size of a settings item value.
 * @param name Name string.
 * @param item_key Item key string.
 * @param size_out Pointer to output the size to.
 */
Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out);

/**
 * @brief Gets the value of a settings item.
 * @param name Name string.
 * @param item_key Item key string.
 * @param value_out Pointer to output the value to.
 * @param value_out_size Size of the value_out buffer.
 * @param size_out Total size which was copied to value_out.
 */
Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size, u64 *size_out);

/**
 * @brief GetTvSettings
 * @param[out] out \ref SetSysTvSettings
 */
Result setsysGetTvSettings(SetSysTvSettings *out);

/**
 * @brief SetTvSettings
 * @param[in] settings \ref SetSysTvSettings
 */
Result setsysSetTvSettings(const SetSysTvSettings *settings);

/**
 * @brief GetEdid
 * @param[out] out \ref SetSysEdid
 */
Result setsysGetEdid(SetSysEdid *out);

/**
 * @brief SetEdid
 * @param[in] edid \ref SetSysEdid
 */
Result setsysSetEdid(const SetSysEdid *edid);

/**
 * @brief GetAudioOutputMode
 * @param[in] target \ref SetSysAudioOutputModeTarget
 * @param[out] out \ref SetSysAudioOutputMode
 */
Result setsysGetAudioOutputMode(SetSysAudioOutputModeTarget target, SetSysAudioOutputMode *out);

/**
 * @brief SetAudioOutputMode
 * @param[in] target \ref SetSysAudioOutputModeTarget
 * @param[in] mode \ref SetSysAudioOutputMode
 */
Result setsysSetAudioOutputMode(SetSysAudioOutputModeTarget target, SetSysAudioOutputMode mode);

/**
 * @brief GetSpeakerAutoMuteFlag
 * @param[out] out Output flag.
 */
Result setsysGetSpeakerAutoMuteFlag(bool *out);

/**
 * @brief SetSpeakerAutoMuteFlag
 * @param[in] flag Input flag.
 */
Result setsysSetSpeakerAutoMuteFlag(bool flag);

/**
 * @brief GetQuestFlag
 * @param[out] out Output flag.
 */
Result setsysGetQuestFlag(bool *out);

/**
 * @brief SetQuestFlag
 * @param[in] flag Input flag.
 */
Result setsysSetQuestFlag(bool flag);

/**
 * @brief GetDataDeletionSettings
 * @param[out] out \ref SetSysDataDeletionSettings
 */
Result setsysGetDataDeletionSettings(SetSysDataDeletionSettings *out);

/**
 * @brief SetDataDeletionSettings
 * @param[in] settings \ref SetSysDataDeletionSettings
 */
Result setsysSetDataDeletionSettings(const SetSysDataDeletionSettings *settings);

/**
 * @brief GetInitialSystemAppletProgramId
 * @param[out] out output ProgramId.
 */
Result setsysGetInitialSystemAppletProgramId(u64 *out);

/**
 * @brief GetOverlayDispProgramId
 * @param[out] out output ProgramId.
 */
Result setsysGetOverlayDispProgramId(u64 *out);

/**
 * @brief GetDeviceTimeZoneLocationName
 * @param[out] out \ref TimeLocationName
 */
Result setsysGetDeviceTimeZoneLocationName(TimeLocationName *out);

/**
 * @brief SetDeviceTimeZoneLocationName
 * @param[in] name \ref TimeLocationName
 */
Result setsysSetDeviceTimeZoneLocationName(const TimeLocationName *name);

/**
 * @brief GetWirelessCertificationFileSize
 * @param[out] out_size Output size.
 */
Result setsysGetWirelessCertificationFileSize(u64 *out_size);

/**
 * @brief GetWirelessCertificationFile
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size.
 * @param[out] out_size Output size.
 */
Result setsysGetWirelessCertificationFile(void* buffer, size_t size, u64 *out_size);

/**
 * @brief SetRegionCode
 * @param[in] region \ref SetRegion
 */
Result setsysSetRegionCode(SetRegion region);

/**
 * @brief GetNetworkSystemClockContext
 * @param[out] out \ref TimeSystemClockContext
 */
Result setsysGetNetworkSystemClockContext(TimeSystemClockContext *out);

/**
 * @brief SetNetworkSystemClockContext
 * @param[in] context \ref TimeSystemClockContext
 */
Result setsysSetNetworkSystemClockContext(const TimeSystemClockContext *context);

/**
 * @brief IsUserSystemClockAutomaticCorrectionEnabled
 * @param[out] out Output flag.
 */
Result setsysIsUserSystemClockAutomaticCorrectionEnabled(bool *out);

/**
 * @brief SetUserSystemClockAutomaticCorrectionEnabled
 * @param[in] flag Input flag.
 */
Result setsysSetUserSystemClockAutomaticCorrectionEnabled(bool flag);

/**
 * @brief GetDebugModeFlag
 * @param[out] out Output flag.
 */
Result setsysGetDebugModeFlag(bool *out);

/**
 * @brief GetPrimaryAlbumStorage
 * @param[out] out \ref GetPrimaryAlbumStorage
 */
Result setsysGetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage *out);

/**
 * @brief SetPrimaryAlbumStorage
 * @param[in] storage \ref SetSysPrimaryAlbumStorage
 */
Result setsysSetPrimaryAlbumStorage(SetSysPrimaryAlbumStorage storage);

/**
 * @brief GetUsb30EnableFlag
 * @param[out] out Output flag.
 */
Result setsysGetUsb30EnableFlag(bool *out);

/**
 * @brief SetUsb30EnableFlag
 * @param[in] flag Input flag.
 */
Result setsysSetUsb30EnableFlag(bool flag);

/**
 * @brief Gets the \ref SetBatteryLot.
 * @param[out] out \ref SetBatteryLot
 */
Result setsysGetBatteryLot(SetBatteryLot *out);

/**
 * @brief Gets the system's serial number.
 * @param[out] out \ref SetSysSerialNumber
 */
Result setsysGetSerialNumber(SetSysSerialNumber *out);

/**
 * @brief GetNfcEnableFlag
 * @param[out] out Output flag.
 */
Result setsysGetNfcEnableFlag(bool *out);

/**
 * @brief SetNfcEnableFlag
 * @param[in] flag Input flag.
 */
Result setsysSetNfcEnableFlag(bool flag);

/**
 * @brief GetSleepSettings
 * @param[out] out \ref SetSysSleepSettings
 */
Result setsysGetSleepSettings(SetSysSleepSettings *out);

/**
 * @brief SetSleepSettings
 * @param[in] settings \ref SetSysSleepSettings
 */
Result setsysSetSleepSettings(const SetSysSleepSettings *settings);

/**
 * @brief GetWirelessLanEnableFlag
 * @param[out] out Output flag.
 */
Result setsysGetWirelessLanEnableFlag(bool *out);

/**
 * @brief SetWirelessLanEnableFlag
 * @param[in] flag Input flag.
 */
Result setsysSetWirelessLanEnableFlag(bool flag);

/**
 * @brief GetInitialLaunchSettings
 * @param[out] out \ref SetSysInitialLaunchSettings
 */
Result setsysGetInitialLaunchSettings(SetSysInitialLaunchSettings *out);

/**
 * @brief SetInitialLaunchSettings
 * @param[in] settings \ref SetSysInitialLaunchSettings
 */
Result setsysSetInitialLaunchSettings(const SetSysInitialLaunchSettings *settings);

/**
 * @brief Gets the system's nickname.
 * @note Same as \ref setGetDeviceNickname, which official sw uses instead on [10.1.0+].
 * @param[out] nickname \ref SetSysDeviceNickName
 */
Result setsysGetDeviceNickname(SetSysDeviceNickName *nickname);

/**
 * @brief Sets the system's nickname.
 * @param[in] nickname \ref SetSysDeviceNickName
 */
Result setsysSetDeviceNickname(const SetSysDeviceNickName *nickname);

/**
 * @brief GetProductModel
 * @param[out] model Output SetSysProductModel.
 */
Result setsysGetProductModel(SetSysProductModel *model);

/**
 * @brief GetLdnChannel
 * @param[out] out Output LdnChannel.
 */
Result setsysGetLdnChannel(s32 *out);

/**
 * @brief SetLdnChannel
 * @param[in] channel Input LdnChannel.
 */
Result setsysSetLdnChannel(s32 channel);

/**
 * @brief Gets an event that settings will signal on flag change.
 * @param out_event Event to bind. Output event will have autoclear=false.
 */
Result setsysAcquireTelemetryDirtyFlagEventHandle(Event *out_event);

/**
 * @brief Gets the settings flags that have changed.
 * @param flags_0 Pointer to populate with first 64 flags.
 * @param flags_1 Pointer to populate with second 64 flags.
 */
Result setsysGetTelemetryDirtyFlags(u64 *flags_0, u64 *flags_1);

/**
 * @brief GetPtmBatteryLot
 * @param[out] out \ref SetBatteryLot
 */
Result setsysGetPtmBatteryLot(SetBatteryLot *out);

/**
 * @brief SetPtmBatteryLot
 * @param[in] lot \ref SetBatteryLot
 */
Result setsysSetPtmBatteryLot(const SetBatteryLot *lot);

/**
 * @brief GetPtmFuelGaugeParameter
 * @param[out] out \ref SetSysPtmFuelGaugeParameter
 */
Result setsysGetPtmFuelGaugeParameter(SetSysPtmFuelGaugeParameter *out);

/**
 * @brief SetPtmFuelGaugeParameter
 * @param[in] parameter \ref SetSysPtmFuelGaugeParameter
 */
Result setsysSetPtmFuelGaugeParameter(const SetSysPtmFuelGaugeParameter *parameter);

/**
 * @brief GetBluetoothEnableFlag
 * @param[out] out Output flag.
 */
Result setsysGetBluetoothEnableFlag(bool *out);

/**
 * @brief SetBluetoothEnableFlag
 * @param[in] flag Input flag.
 */
Result setsysSetBluetoothEnableFlag(bool flag);

/**
 * @brief GetMiiAuthorId
 * @param[out] out Output MiiAuthorId.
 */
Result setsysGetMiiAuthorId(Uuid *out);

/**
 * @brief SetShutdownRtcValue
 * @param[in] value Input value.
 */
Result setsysSetShutdownRtcValue(u64 value);

/**
 * @brief GetShutdownRtcValue
 * @param[out] out Output value.
 */
Result setsysGetShutdownRtcValue(u64 *out);

/**
 * @brief Gets an event that settings will signal on flag change.
 * @param out_event Event to bind. Output event will have autoclear=false.
 */
Result setsysAcquireFatalDirtyFlagEventHandle(Event *out_event);

/**
 * @brief Gets the settings flags that have changed.
 * @param flags_0 Pointer to populate with first 64 flags.
 * @param flags_1 Pointer to populate with second 64 flags.
 */
Result setsysGetFatalDirtyFlags(u64 *flags_0, u64 *flags_1);

/**
 * @brief GetAutoUpdateEnableFlag
 * @note Only available on [2.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetAutoUpdateEnableFlag(bool *out);

/**
 * @brief SetAutoUpdateEnableFlag
 * @note Only available on [2.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetAutoUpdateEnableFlag(bool flag);

/**
 * @brief GetNxControllerSettings
 * @note On [13.0.0+] \ref setsysGetNxControllerSettingsEx should be used instead.
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysNxControllerLegacySettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetNxControllerSettings(s32 *total_out, SetSysNxControllerLegacySettings *settings, s32 count);

/**
 * @brief SetNxControllerSettings
 * @note On [13.0.0+] \ref setsysSetNxControllerSettingsEx should be used instead.
 * @param[in] settings Input array of \ref SetSysNxControllerLegacySettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetNxControllerSettings(const SetSysNxControllerLegacySettings *settings, s32 count);

/**
 * @brief GetBatteryPercentageFlag
 * @note Only available on [2.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetBatteryPercentageFlag(bool *out);

/**
 * @brief SetBatteryPercentageFlag
 * @note Only available on [2.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetBatteryPercentageFlag(bool flag);

/**
 * @brief GetExternalRtcResetFlag
 * @note Only available on [2.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetExternalRtcResetFlag(bool *out);

/**
 * @brief SetExternalRtcResetFlag
 * @note Only available on [2.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetExternalRtcResetFlag(bool flag);

/**
 * @brief GetUsbFullKeyEnableFlag
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetUsbFullKeyEnableFlag(bool *out);

/**
 * @brief SetUsbFullKeyEnableFlag
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetUsbFullKeyEnableFlag(bool flag);

/**
 * @brief SetExternalSteadyClockInternalOffset
 * @note Only available on [3.0.0+].
 * @param[in] offset Input offset.
 */
Result setsysSetExternalSteadyClockInternalOffset(u64 offset);

/**
 * @brief GetExternalSteadyClockInternalOffset
 * @note Only available on [3.0.0+].
 * @param[out] out Output offset.
 */
Result setsysGetExternalSteadyClockInternalOffset(u64 *out);

/**
 * @brief GetBacklightSettingsEx
 * @note Only available on [3.0.0+].
 * @param[out] out \ref SetSysBacklightSettingsEx
 */
Result setsysGetBacklightSettingsEx(SetSysBacklightSettingsEx *out);

/**
 * @brief SetBacklightSettingsEx
 * @note Only available on [3.0.0+].
 * @param[in] settings \ref SetSysBacklightSettingsEx
 */
Result setsysSetBacklightSettingsEx(const SetSysBacklightSettingsEx *settings);

/**
 * @brief GetHeadphoneVolumeWarningCount
 * @note Only available on [3.0.0+].
 * @param[out] out Output count.
 */
Result setsysGetHeadphoneVolumeWarningCount(u32 *out);

/**
 * @brief SetHeadphoneVolumeWarningCount
 * @note Only available on [3.0.0+].
 * @param[in] count Input count.
 */
Result setsysSetHeadphoneVolumeWarningCount(u32 count);

/**
 * @brief GetBluetoothAfhEnableFlag
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetBluetoothAfhEnableFlag(bool *out);

/**
 * @brief SetBluetoothAfhEnableFlag
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetBluetoothAfhEnableFlag(bool flag);

/**
 * @brief GetBluetoothBoostEnableFlag
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetBluetoothBoostEnableFlag(bool *out);

/**
 * @brief SetBluetoothBoostEnableFlag
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetBluetoothBoostEnableFlag(bool flag);

/**
 * @brief GetInRepairProcessEnableFlag
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetInRepairProcessEnableFlag(bool *out);

/**
 * @brief SetInRepairProcessEnableFlag
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetInRepairProcessEnableFlag(bool flag);

/**
 * @brief GetHeadphoneVolumeUpdateFlag
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetHeadphoneVolumeUpdateFlag(bool *out);

/**
 * @brief SetHeadphoneVolumeUpdateFlag
 * @note Only available on [3.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetHeadphoneVolumeUpdateFlag(bool flag);

/**
 * @brief NeedsToUpdateHeadphoneVolume
 * @note Only available on [3.0.0-14.1.2].
 * @param[out] a0 Output arg.
 * @param[out] a1 Output arg.
 * @param[out] a2 Output arg.
 * @param[in] flag Input flag.
 */
Result setsysNeedsToUpdateHeadphoneVolume(u8 *a0, u8 *a1, u8 *a2, bool flag);

/**
 * @brief GetPushNotificationActivityModeOnSleep
 * @note Only available on [3.0.0+].
 * @param[out] out Output mode.
 */
Result setsysGetPushNotificationActivityModeOnSleep(u32 *out);

/**
 * @brief SetPushNotificationActivityModeOnSleep
 * @note Only available on [3.0.0+].
 * @param[in] mode Input mode.
 */
Result setsysSetPushNotificationActivityModeOnSleep(u32 mode);

/**
 * @brief GetServiceDiscoveryControlSettings
 * @note Only available on [4.0.0+].
 * @param[out] out \ref ServiceDiscoveryControlSettings
 */
Result setsysGetServiceDiscoveryControlSettings(SetSysServiceDiscoveryControlSettings *out);

/**
 * @brief SetServiceDiscoveryControlSettings
 * @note Only available on [4.0.0+].
 * @param[in] settings \ref ServiceDiscoveryControlSettings
 */
Result setsysSetServiceDiscoveryControlSettings(SetSysServiceDiscoveryControlSettings settings);

/**
 * @brief GetErrorReportSharePermission
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysErrorReportSharePermission
 */
Result setsysGetErrorReportSharePermission(SetSysErrorReportSharePermission *out);

/**
 * @brief SetErrorReportSharePermission
 * @note Only available on [4.0.0+].
 * @param[in] permission \ref SetSysErrorReportSharePermission
 */
Result setsysSetErrorReportSharePermission(SetSysErrorReportSharePermission permission);

/**
 * @brief GetAppletLaunchFlags
 * @note Only available on [4.0.0+].
 * @param[out] out Output AppletLaunchFlags bitmask.
 */
Result setsysGetAppletLaunchFlags(u32 *out);

/**
 * @brief SetAppletLaunchFlags
 * @note Only available on [4.0.0+].
 * @param[in] flags Input AppletLaunchFlags bitmask.
 */
Result setsysSetAppletLaunchFlags(u32 flags);

/**
 * @brief GetConsoleSixAxisSensorAccelerationBias
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAccelerationBias
 */
Result setsysGetConsoleSixAxisSensorAccelerationBias(SetSysConsoleSixAxisSensorAccelerationBias *out);

/**
 * @brief SetConsoleSixAxisSensorAccelerationBias
 * @note Only available on [4.0.0+].
 * @param[in] bias \ref SetSysConsoleSixAxisSensorAccelerationBias
 */
Result setsysSetConsoleSixAxisSensorAccelerationBias(const SetSysConsoleSixAxisSensorAccelerationBias *bias);

/**
 * @brief GetConsoleSixAxisSensorAngularVelocityBias
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAngularVelocityBias
 */
Result setsysGetConsoleSixAxisSensorAngularVelocityBias(SetSysConsoleSixAxisSensorAngularVelocityBias *out);

/**
 * @brief SetConsoleSixAxisSensorAngularVelocityBias
 * @note Only available on [4.0.0+].
 * @param[in] bias \ref SetSysConsoleSixAxisSensorAngularVelocityBias
 */
Result setsysSetConsoleSixAxisSensorAngularVelocityBias(const SetSysConsoleSixAxisSensorAngularVelocityBias *bias);

/**
 * @brief GetConsoleSixAxisSensorAccelerationGain
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAccelerationGain
 */
Result setsysGetConsoleSixAxisSensorAccelerationGain(SetSysConsoleSixAxisSensorAccelerationGain *out);

/**
 * @brief SetConsoleSixAxisSensorAccelerationGain
 * @note Only available on [4.0.0+].
 * @param[in] gain \ref SetSysConsoleSixAxisSensorAccelerationGain
 */
Result setsysSetConsoleSixAxisSensorAccelerationGain(const SetSysConsoleSixAxisSensorAccelerationGain *gain);

/**
 * @brief GetConsoleSixAxisSensorAngularVelocityGain
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAngularVelocityGain
 */
Result setsysGetConsoleSixAxisSensorAngularVelocityGain(SetSysConsoleSixAxisSensorAngularVelocityGain *out);

/**
 * @brief SetConsoleSixAxisSensorAngularVelocityGain
 * @note Only available on [4.0.0+].
 * @param[in] gain \ref SetSysConsoleSixAxisSensorAngularVelocityGain
 */
Result setsysSetConsoleSixAxisSensorAngularVelocityGain(const SetSysConsoleSixAxisSensorAngularVelocityGain *gain);

/**
 * @brief GetKeyboardLayout
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetKeyboardLayout
 */
Result setsysGetKeyboardLayout(SetKeyboardLayout *out);

/**
 * @brief SetKeyboardLayout
 * @note Only available on [4.0.0+].
 * @param[in] layout \ref SetKeyboardLayout
 */
Result setsysSetKeyboardLayout(SetKeyboardLayout layout);

/**
 * @brief GetWebInspectorFlag
 * @note Only available on [4.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetWebInspectorFlag(bool *out);

/**
 * @brief GetAllowedSslHosts
 * @note Only available on [4.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] out Output array of \ref SetSysAllowedSslHosts.
 * @param[in] count Size of the hosts array in entries.
 */
Result setsysGetAllowedSslHosts(s32 *total_out, SetSysAllowedSslHosts *out, s32 count);

/**
 * @brief GetHostFsMountPoint
 * @note Only available on [4.0.0+].
 * @param[out] out \ref SetSysHostFsMountPoint
 */
Result setsysGetHostFsMountPoint(SetSysHostFsMountPoint *out);

/**
 * @brief GetRequiresRunRepairTimeReviser
 * @note Only available on [5.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetRequiresRunRepairTimeReviser(bool *out);

/**
 * @brief SetRequiresRunRepairTimeReviser
 * @note Only available on [5.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetRequiresRunRepairTimeReviser(bool flag);

/**
 * @brief SetBlePairingSettings
 * @note Only available on [5.0.0+].
 * @param[in] settings Input array of \ref SetSysBlePairingSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetBlePairingSettings(const SetSysBlePairingSettings *settings, s32 count);

/**
 * @brief GetBlePairingSettings
 * @note Only available on [5.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysBlePairingSettings.
 * @param[in] count Size of the hosts array in entries.
 */
Result setsysGetBlePairingSettings(s32 *total_out, SetSysBlePairingSettings *settings, s32 count);

/**
 * @brief GetConsoleSixAxisSensorAngularVelocityTimeBias
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAngularVelocityTimeBias
 */
Result setsysGetConsoleSixAxisSensorAngularVelocityTimeBias(SetSysConsoleSixAxisSensorAngularVelocityTimeBias *out);

/**
 * @brief SetConsoleSixAxisSensorAngularVelocityTimeBias
 * @note Only available on [5.0.0+].
 * @param[in] bias \ref SetSysConsoleSixAxisSensorAngularVelocityTimeBias
 */
Result setsysSetConsoleSixAxisSensorAngularVelocityTimeBias(const SetSysConsoleSixAxisSensorAngularVelocityTimeBias *bias);

/**
 * @brief GetConsoleSixAxisSensorAngularAcceleration
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetSysConsoleSixAxisSensorAngularAcceleration
 */
Result setsysGetConsoleSixAxisSensorAngularAcceleration(SetSysConsoleSixAxisSensorAngularAcceleration *out);

/**
 * @brief SetConsoleSixAxisSensorAngularAcceleration
 * @note Only available on [5.0.0+].
 * @param[in] acceleration \ref SetSysConsoleSixAxisSensorAngularAcceleration
 */
Result setsysSetConsoleSixAxisSensorAngularAcceleration(const SetSysConsoleSixAxisSensorAngularAcceleration *acceleration);

/**
 * @brief GetRebootlessSystemUpdateVersion
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetSysRebootlessSystemUpdateVersion
 */
Result setsysGetRebootlessSystemUpdateVersion(SetSysRebootlessSystemUpdateVersion *out);

/**
 * @brief GetDeviceTimeZoneLocationUpdatedTime
 * @note Only available on [5.0.0+].
 * @param[out] out \ref TimeSteadyClockTimePoint
 */
Result setsysGetDeviceTimeZoneLocationUpdatedTime(TimeSteadyClockTimePoint *out);

/**
 * @brief SetDeviceTimeZoneLocationUpdatedTime
 * @note Only available on [5.0.0+].
 * @param[in] time_point \ref TimeSteadyClockTimePoint
 */
Result setsysSetDeviceTimeZoneLocationUpdatedTime(const TimeSteadyClockTimePoint *time_point);

/**
 * @brief GetUserSystemClockAutomaticCorrectionUpdatedTime
 * @note Only available on [6.0.0+].
 * @param[out] out \ref TimeSteadyClockTimePoint
 */
Result setsysGetUserSystemClockAutomaticCorrectionUpdatedTime(TimeSteadyClockTimePoint *out);

/**
 * @brief SetUserSystemClockAutomaticCorrectionUpdatedTime
 * @note Only available on [6.0.0+].
 * @param[in] time_point \ref TimeSteadyClockTimePoint
 */
Result setsysSetUserSystemClockAutomaticCorrectionUpdatedTime(const TimeSteadyClockTimePoint *time_point);

/**
 * @brief GetAccountOnlineStorageSettings
 * @note Only available on [6.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysAccountOnlineStorageSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetAccountOnlineStorageSettings(s32 *total_out, SetSysAccountOnlineStorageSettings *settings, s32 count);

/**
 * @brief SetAccountOnlineStorageSettings
 * @note Only available on [6.0.0+].
 * @param[in] settings Input array of \ref SetSysAccountOnlineStorageSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetAccountOnlineStorageSettings(const SetSysAccountOnlineStorageSettings *settings, s32 count);

/**
 * @brief GetPctlReadyFlag
 * @note Only available on [6.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetPctlReadyFlag(bool *out);

/**
 * @brief SetPctlReadyFlag
 * @note Only available on [6.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetPctlReadyFlag(bool flag);

/**
 * @brief GetAnalogStickUserCalibrationL
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetSysAnalogStickUserCalibration
 */
Result setsysGetAnalogStickUserCalibrationL(SetSysAnalogStickUserCalibration *out);

/**
 * @brief SetAnalogStickUserCalibrationL
 * @note Only available on [8.1.1+].
 * @param[in] calibration \ref SetSysAnalogStickUserCalibration
 */
Result setsysSetAnalogStickUserCalibrationL(const SetSysAnalogStickUserCalibration *calibration);

/**
 * @brief GetAnalogStickUserCalibrationR
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetSysAnalogStickUserCalibration
 */
Result setsysGetAnalogStickUserCalibrationR(SetSysAnalogStickUserCalibration *out);

/**
 * @brief SetAnalogStickUserCalibrationR
 * @note Only available on [8.1.1+].
 * @param[in] calibration \ref SetSysAnalogStickUserCalibration
 */
Result setsysSetAnalogStickUserCalibrationR(const SetSysAnalogStickUserCalibration *calibration);

/**
 * @brief GetPtmBatteryVersion
 * @note Only available on [6.0.0+].
 * @param[out] out Output version.
 */
Result setsysGetPtmBatteryVersion(u8 *out);

/**
 * @brief SetPtmBatteryVersion
 * @note Only available on [6.0.0+].
 * @param[in] version Input version.
 */
Result setsysSetPtmBatteryVersion(u8 version);

/**
 * @brief GetUsb30HostEnableFlag
 * @note Only available on [6.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetUsb30HostEnableFlag(bool *out);

/**
 * @brief SetUsb30HostEnableFlag
 * @note Only available on [6.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetUsb30HostEnableFlag(bool flag);

/**
 * @brief GetUsb30DeviceEnableFlag
 * @note Only available on [6.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetUsb30DeviceEnableFlag(bool *out);

/**
 * @brief SetUsb30DeviceEnableFlag
 * @note Only available on [6.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetUsb30DeviceEnableFlag(bool flag);

/**
 * @brief GetThemeId
 * @note Only available on [7.0.0+].
 * @param[in] type Input theme id type.
 * @param[out] out \ref SetSysThemeId
 */
Result setsysGetThemeId(s32 type, SetSysThemeId *out);

/**
 * @brief SetThemeId
 * @note Only available on [7.0.0+].
 * @param[in] type Input theme id type.
 * @param[in] theme_id \ref SetSysThemeId
 */
Result setsysSetThemeId(s32 type, const SetSysThemeId *theme_id);

/**
 * @brief GetChineseTraditionalInputMethod
 * @note Only available on [7.0.0+].
 * @param[out] out \ref SetChineseTraditionalInputMethod
 */
Result setsysGetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod *out);

/**
 * @brief SetChineseTraditionalInputMethod
 * @note Only available on [7.0.0+].
 * @param[in] method \ref SetChineseTraditionalInputMethod
 */
Result setsysSetChineseTraditionalInputMethod(SetChineseTraditionalInputMethod method);

/**
 * @brief GetPtmCycleCountReliability
 * @note Only available on [7.0.0+].
 * @param[out] out \ref SetSysPtmCycleCountReliability
 */
Result setsysGetPtmCycleCountReliability(SetSysPtmCycleCountReliability *out);

/**
 * @brief SetPtmCycleCountReliability
 * @note Only available on [7.0.0+].
 * @param[in] reliability \ref SetSysPtmCycleCountReliability
 */
Result setsysSetPtmCycleCountReliability(SetSysPtmCycleCountReliability reliability);

/**
 * @brief Gets the \ref SetSysHomeMenuScheme.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetSysHomeMenuScheme
 */
Result setsysGetHomeMenuScheme(SetSysHomeMenuScheme *out);

/**
 * @brief GetThemeSettings
 * @note Only available on [7.0.0+].
 * @param[out] out \ref SetSysThemeSettings
 */
Result setsysGetThemeSettings(SetSysThemeSettings *out);

/**
 * @brief SetThemeSettings
 * @note Only available on [7.0.0+].
 * @param[in] settings \ref SetSysThemeSettings
 */
Result setsysSetThemeSettings(const SetSysThemeSettings *settings);

/**
 * @brief GetThemeKey
 * @note Only available on [7.0.0+].
 * @param[out] out \ref FsArchiveMacKey
 */
Result setsysGetThemeKey(FsArchiveMacKey *out);

/**
 * @brief SetThemeKey
 * @note Only available on [7.0.0+].
 * @param[in] key \ref FsArchiveMacKey
 */
Result setsysSetThemeKey(const FsArchiveMacKey *key);

/**
 * @brief GetZoomFlag
 * @note Only available on [8.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetZoomFlag(bool *out);

/**
 * @brief SetZoomFlag
 * @note Only available on [8.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetZoomFlag(bool flag);

/**
 * @brief Returns Terra platform type flag.
 * @note On [9.0.0+], this is a wrapper for \ref setsysGetPlatFormRegion() == 2.
 * @note Only available on [8.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetT(bool *out);

/**
 * @brief Sets Terra platform type flag.
 * @note On [9.0.0+], this is a wrapper for \ref setsysSetPlatFormRegion(1 + (IsT & 1)).
 * @note Only available on [8.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetT(bool flag);

/**
 * @brief Gets the \ref SetSysPlatformRegion.
 * @note This is used internally by \ref appletGetSettingsPlatformRegion.
 * @note Only available on [9.0.0+].
 * @param[out] out \ref SetSysPlatformRegion
 */
Result setsysGetPlatformRegion(SetSysPlatformRegion *out);

/**
 * @brief Sets the \ref SetSysPlatformRegion.
 * @note Only available on [9.0.0+].
 * @param[in] region \ref SetSysPlatformRegion
 */
Result setsysSetPlatformRegion(SetSysPlatformRegion region);

/**
 * @brief GetHomeMenuSchemeModel
 * @note This will throw an error when loading the "settings_debug!{...}" system-setting which is used with this fails.
 * @note Only available on [9.0.0+].
 * @param[out] out HomeMenuSchemeModel.
 */
Result setsysGetHomeMenuSchemeModel(u32 *out);

/**
 * @brief GetMemoryUsageRateFlag
 * @note Only available on [9.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetMemoryUsageRateFlag(bool *out);

/**
 * @brief Gets the \ref SetSysTouchScreenMode.
 * @note Only available on [9.0.0+].
 * @param[out] out \ref SetSysTouchScreenMode
 */
Result setsysGetTouchScreenMode(SetSysTouchScreenMode *out);

/**
 * @brief Sets the \ref SetSysTouchScreenMode.
 * @note Only available on [9.0.0+].
 * @param[in] mode \ref SetSysTouchScreenMode
 */
Result setsysSetTouchScreenMode(SetSysTouchScreenMode mode);

/**
 * @brief GetButtonConfigSettingsFull
 * @note Only available on [10.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetButtonConfigSettingsFull(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief SetButtonConfigSettingsFull
 * @note Only available on [10.0.0+].
 * @param[in] settings Input array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetButtonConfigSettingsFull(const SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief GetButtonConfigSettingsEmbedded
 * @note Only available on [10.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetButtonConfigSettingsEmbedded(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief SetButtonConfigSettingsEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] settings Input array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetButtonConfigSettingsEmbedded(const SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief GetButtonConfigSettingsLeft
 * @note Only available on [10.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetButtonConfigSettingsLeft(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief SetButtonConfigSettingsLeft
 * @note Only available on [10.0.0+].
 * @param[in] settings Input array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetButtonConfigSettingsLeft(const SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief GetButtonConfigSettingsRight
 * @note Only available on [10.0.0+].
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetButtonConfigSettingsRight(s32 *total_out, SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief SetButtonConfigSettingsRight
 * @note Only available on [10.0.0+].
 * @param[in] settings Input array of \ref SetSysButtonConfigSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetButtonConfigSettingsRight(const SetSysButtonConfigSettings *settings, s32 count);

/**
 * @brief GetButtonConfigRegisteredSettingsEmbedded
 * @note Only available on [10.0.0+].
 * @param[out] settings \ref SetSysButtonConfigRegisteredSettings
 */
Result setsysGetButtonConfigRegisteredSettingsEmbedded(SetSysButtonConfigRegisteredSettings *settings);

/**
 * @brief SetButtonConfigRegisteredSettingsEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] settings \ref SetSysButtonConfigRegisteredSettings
 */
Result setsysSetButtonConfigRegisteredSettingsEmbedded(const SetSysButtonConfigRegisteredSettings *settings);

/**
 * @brief GetButtonConfigRegisteredSettings
 * @note Only available on [10.0.0+].
 * @param[out] settings \ref SetSysButtonConfigRegisteredSettings
 */
Result setsysGetButtonConfigRegisteredSettings(s32 *total_out, SetSysButtonConfigRegisteredSettings *settings, s32 count);

/**
 * @brief SetButtonConfigRegisteredSettings
 * @note Only available on [10.0.0+].
 * @param[in] settings \ref SetSysButtonConfigRegisteredSettings
 */
Result setsysSetButtonConfigRegisteredSettings(const SetSysButtonConfigRegisteredSettings *settings, s32 count);

/**
 * @brief GetFieldTestingFlag
 * @note Only available on [10.1.0+].
 * @param[out] out Output flag.
 */
Result setsysGetFieldTestingFlag(bool *out);

/**
 * @brief SetFieldTestingFlag
 * @note Only available on [10.1.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetFieldTestingFlag(bool flag);

/**
 * @brief GetNxControllerSettingsEx
 * @param[out] total_out Total output entries.
 * @param[out] settings Output array of \ref SetSysNxControllerSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysGetNxControllerSettingsEx(s32 *total_out, SetSysNxControllerSettings *settings, s32 count);

/**
 * @brief SetNxControllerSettingsEx
 * @param[in] settings Input array of \ref SetSysNxControllerSettings.
 * @param[in] count Size of the settings array in entries.
 */
Result setsysSetNxControllerSettingsEx(const SetSysNxControllerSettings *settings, s32 count);

/// Initialize setcal.
Result setcalInitialize(void);

/// Exit setcal.
void setcalExit(void);

/// Gets the Service object for the actual setcal service session.
Service* setcalGetServiceSession(void);

/**
 * @brief Gets the \ref SetCalBdAddress.
 * @param[out] out \ref SetCalBdAddress
 */
Result setcalGetBdAddress(SetCalBdAddress *out);

/**
 * @brief Gets the \ref SetCalConfigurationId1.
 * @param[out] out \ref SetCalConfigurationId1
 */
Result setcalGetConfigurationId1(SetCalConfigurationId1 *out);

/**
 * @brief Gets the \ref SetCalAccelerometerOffset.
 * @param[out] out \ref SetCalAccelerometerOffset
 */
Result setcalGetAccelerometerOffset(SetCalAccelerometerOffset *out);

/**
 * @brief Gets the \ref SetCalAccelerometerScale.
 * @param[out] out \ref SetCalAccelerometerScale
 */
Result setcalGetAccelerometerScale(SetCalAccelerometerScale *out);

/**
 * @brief Gets the \ref SetCalGyroscopeOffset.
 * @param[out] out \ref SetCalGyroscopeOffset
 */
Result setcalGetGyroscopeOffset(SetCalGyroscopeOffset *out);

/**
 * @brief Gets the \ref SetCalGyroscopeScale.
 * @param[out] out \ref SetCalGyroscopeScale
 */
Result setcalGetGyroscopeScale(SetCalGyroscopeScale *out);

/**
 * @brief Gets the \ref SetCalMacAddress.
 * @param[out] out \ref SetCalMacAddress
 */
Result setcalGetWirelessLanMacAddress(SetCalMacAddress *out);

/**
 * @brief GetWirelessLanCountryCodeCount
 * @param[out] out_count Output count
 */
Result setcalGetWirelessLanCountryCodeCount(s32 *out_count);

/**
 * @brief GetWirelessLanCountryCodes
 * @param[out] total_out Total output entries.
 * @param[out] codes Output array of \ref SetCalCountryCode.
 * @param[in] count Size of the versions array in entries.
 */
Result setcalGetWirelessLanCountryCodes(s32 *total_out, SetCalCountryCode *codes, s32 count);

/**
 * @brief Gets the \ref SetCalSerialNumber.
 * @param[out] out \ref SetCalSerialNumber
 */
Result setcalGetSerialNumber(SetCalSerialNumber *out);

/**
 * @brief SetInitialSystemAppletProgramId
 * @param[in] program_id input ProgramId.
 */
Result setcalSetInitialSystemAppletProgramId(u64 program_id);

/**
 * @brief SetOverlayDispProgramId
 * @param[in] program_id input ProgramId.
 */
Result setcalSetOverlayDispProgramId(u64 program_id);

/**
 * @brief Gets the \ref SetBatteryLot.
 * @param[out] out \ref SetBatteryLot
 */
Result setcalGetBatteryLot(SetBatteryLot *out);

/**
 * @brief Gets the \ref SetCalEccB233DeviceCertificate.
 * @param[out] out \ref SetCalEccB233DeviceCertificate
 */
Result setcalGetEciDeviceCertificate(SetCalEccB233DeviceCertificate *out);

/**
 * @brief Gets the \ref SetCalRsa2048DeviceCertificate.
 * @param[out] out \ref SetCalRsa2048DeviceCertificate
 */
Result setcalGetEticketDeviceCertificate(SetCalRsa2048DeviceCertificate *out);

/**
 * @brief Gets the \ref SetCalSslKey.
 * @param[out] out \ref SetCalSslKey
 */
Result setcalGetSslKey(SetCalSslKey *out);

/**
 * @brief Gets the \ref SetCalSslCertificate.
 * @param[out] out \ref SetCalSslCertificate
 */
Result setcalGetSslCertificate(SetCalSslCertificate *out);

/**
 * @brief Gets the \ref SetCalGameCardKey.
 * @param[out] out \ref SetCalGameCardKey
 */
Result setcalGetGameCardKey(SetCalGameCardKey *out);

/**
 * @brief Gets the \ref SetCalGameCardCertificate.
 * @param[out] out \ref SetCalGameCardCertificate
 */
Result setcalGetGameCardCertificate(SetCalGameCardCertificate *out);

/**
 * @brief Gets the \ref SetCalEccB233DeviceKey.
 * @param[out] out \ref SetCalEccB233DeviceKey
 */
Result setcalGetEciDeviceKey(SetCalEccB233DeviceKey *out);

/**
 * @brief Gets the \ref SetCalRsa2048DeviceKey.
 * @param[out] out \ref SetCalRsa2048DeviceKey
 */
Result setcalGetEticketDeviceKey(SetCalRsa2048DeviceKey *out);

/**
 * @brief Gets the \ref SetCalSpeakerParameter.
 * @param[out] out \ref SetCalSpeakerParameter
 */
Result setcalGetSpeakerParameter(SetCalSpeakerParameter *out);

/**
 * @brief GetLcdVendorId
 * @note Only available on [4.0.0+].
 * @param[out] out_vendor_id Output LcdVendorId.
 */
Result setcalGetLcdVendorId(u32 *out_vendor_id);

/**
 * @brief Gets the \ref SetCalRsa2048DeviceCertificate.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalRsa2048DeviceCertificate
 */
Result setcalGetEciDeviceCertificate2(SetCalRsa2048DeviceCertificate *out);

/**
 * @brief Gets the \ref SetCalRsa2048DeviceKey.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalRsa2048DeviceKey
 */
Result setcalGetEciDeviceKey2(SetCalRsa2048DeviceKey *out);

/**
 * @brief Gets the \ref SetCalAmiiboKey.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboKey
 */
Result setcalGetAmiiboKey(SetCalAmiiboKey *out);

/**
 * @brief Gets the \ref SetCalAmiiboEcqvCertificate.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboEcqvCertificate
 */
Result setcalGetAmiiboEcqvCertificate(SetCalAmiiboEcqvCertificate *out);

/**
 * @brief Gets the \ref SetCalAmiiboEcdsaCertificate.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboEcdsaCertificate
 */
Result setcalGetAmiiboEcdsaCertificate(SetCalAmiiboEcdsaCertificate *out);

/**
 * @brief Gets the \ref SetCalAmiiboEcqvBlsKey.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboEcqvBlsKey
 */
Result setcalGetAmiiboEcqvBlsKey(SetCalAmiiboEcqvBlsKey *out);

/**
 * @brief Gets the \ref SetCalAmiiboEcqvBlsCertificate.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboEcqvBlsCertificate
 */
Result setcalGetAmiiboEcqvBlsCertificate(SetCalAmiiboEcqvBlsCertificate *out);

/**
 * @brief Gets the \ref SetCalAmiiboEcqvBlsRootCertificate.
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetCalAmiiboEcqvBlsRootCertificate
 */
Result setcalGetAmiiboEcqvBlsRootCertificate(SetCalAmiiboEcqvBlsRootCertificate *out);

/**
 * @brief GetUsbTypeCPowerSourceCircuitVersion
 * @note Only available on [5.0.0+].
 * @param[out] out_version Output UsbTypeCPowerSourceCircuitVersion.
 */
Result setcalGetUsbTypeCPowerSourceCircuitVersion(u8 *out_version);

/**
 * @brief GetAnalogStickModuleTypeL
 * @note Only available on [8.1.1+].
 * @param[out] out_version Output AnalogStickModuleType.
 */
Result setcalGetAnalogStickModuleTypeL(u8 *out_type);

/**
 * @brief Gets the \ref SetCalAnalogStickModelParameter.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetCalAnalogStickModelParameter
 */
Result setcalGetAnalogStickModelParameterL(SetCalAnalogStickModelParameter *out);

/**
 * @brief Gets the \ref SetCalAnalogStickFactoryCalibration.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetCalAnalogStickFactoryCalibration
 */
Result setcalGetAnalogStickFactoryCalibrationL(SetCalAnalogStickFactoryCalibration *out);

/**
 * @brief GetAnalogStickModuleTypeR
 * @note Only available on [8.1.1+].
 * @param[out] out_version Output AnalogStickModuleType.
 */
Result setcalGetAnalogStickModuleTypeR(u8 *out_type);

/**
 * @brief Gets the \ref SetCalAnalogStickModelParameter.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetCalAnalogStickModelParameter
 */
Result setcalGetAnalogStickModelParameterR(SetCalAnalogStickModelParameter *out);

/**
 * @brief Gets the \ref SetCalAnalogStickFactoryCalibration.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetCalAnalogStickFactoryCalibration
 */
Result setcalGetAnalogStickFactoryCalibrationR(SetCalAnalogStickFactoryCalibration *out);

/**
 * @brief GetConsoleSixAxisSensorModuleType
 * @note Only available on [8.1.1+].
 * @param[out] out_version Output ConsoleSixAxisSensorModuleType.
 */
Result setcalGetConsoleSixAxisSensorModuleType(u8 *out_type);

/**
 * @brief Gets the \ref SetCalConsoleSixAxisSensorHorizontalOffset.
 * @note Only available on [8.1.1+].
 * @param[out] out \ref SetCalConsoleSixAxisSensorHorizontalOffset
 */
Result setcalGetConsoleSixAxisSensorHorizontalOffset(SetCalConsoleSixAxisSensorHorizontalOffset *out);

/**
 * @brief GetBatteryVersion
 * @note Only available on [6.0.0+].
 * @param[out] out_version Output BatteryVersion.
 */
Result setcalGetBatteryVersion(u8 *out_version);

/**
 * @brief GetDeviceId
 * @note Only available on [10.0.0+].
 * @param[out] out_type Output DeviceId.
 */
Result setcalGetDeviceId(u64 *out_device_id);

/**
 * @brief GetConsoleSixAxisSensorMountType
 * @note Only available on [10.0.0+].
 * @param[out] out_type Output ConsoleSixAxisSensorMountType.
 */
Result setcalGetConsoleSixAxisSensorMountType(u8 *out_type);
