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
#include "../sf/service.h"

#define SET_MAX_NAME_SIZE 0x48
#define SET_MAX_NICKNAME_SIZE 0x80

typedef enum {
    ColorSetId_Light = 0,
    ColorSetId_Dark  = 1,
} ColorSetId;

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
    SetLanguage_Total,       ///< Total languages supported by this enum.
} SetLanguage;

/// Region codes.
typedef enum {
    SetRegion_JPN = 0, ///< Japan
    SetRegion_USA = 1, ///< The Americas
    SetRegion_EUR = 2, ///< Europe
    SetRegion_AUS = 3, ///< Australia/New Zealand
    SetRegion_CHN = 4, ///< China
    SetRegion_KOR = 5, ///< Korea
    SetRegion_TWN = 6, ///< Taiwan
} SetRegion;

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

/// BatteryLot
typedef struct {
    char lot[0x18];    ///< BatteryLot string.
} SetBatteryLot;

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

/// UserSelectorSettings
typedef struct {
    u32 flags;                              ///< Bitmask with \ref SetSysUserSelectorFlag.
} SetSysUserSelectorSettings;

/// AccountSettings
typedef struct {
    SetSysUserSelectorSettings settings;
} SetSysAccountSettings;

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

/// RebootlessSystemUpdateVersion. This is the content of the RebootlessSystemUpdateVersion SystemData, in the "/version" file.
typedef struct {
    u32 version;
    u8 reserved[0x1c];
    char display_version[0x20];
} SetSysRebootlessSystemUpdateVersion;

/// Output from \ref setsysGetHomeMenuScheme. This contains RGBA8 colors which correspond with the physical shell of the system.
typedef struct {
    u32 main_color;       ///< Main Color.
    u32 back_color;       ///< Back Color.
    u32 sub_color;        ///< Sub Color.
    u32 bezel_color;      ///< Bezel Color.
    u32 extra_color;      ///< Extra Color.
} SetSysHomeMenuScheme;

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
 * @brief Gets the system firmware version.
 * @param[out] out Firmware version to populate.
 */
Result setsysGetFirmwareVersion(SetSysFirmwareVersion *out);

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
 * @param serial Pointer to output the serial to. (The buffer size needs to be at least 0x19 bytes)
 */
Result setsysGetSerialNumber(char *serial);

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
 * @param nickname Pointer to output the nickname to. (The buffer size needs to be at least 0x80 bytes)
 */
Result setsysGetDeviceNickname(char* nickname);

/**
 * @brief Sets the system's nickname.
 * @param nickname Pointer to read the nickname from.
 */
Result setsysSetDeviceNickname(const char* nickname);

/**
 * @brief GetProductModel
 * @param[out] out Output ProductModel.
 */
Result setsysGetProductModel(s32 *out);

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
 * @brief Gets an event that settings will signal on flag change.
 * @param out_event Event to bind. Output event will have autoclear=false.
 */
Result setsysBindFatalDirtyFlagEvent(Event *out_event);

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
 * @brief GetRequiresRunRepairTimeReviser
 * @note Only available on [5.0.0+].
 * @param[out] out Output flag.
 */
Result setsysGetRequiresRunRepairTimeReviser(bool *out);

/**
 * @brief GetRebootlessSystemUpdateVersion
 * @note Only available on [5.0.0+].
 * @param[out] out \ref SetSysRebootlessSystemUpdateVersion
 */
Result setsysGetRebootlessSystemUpdateVersion(SetSysRebootlessSystemUpdateVersion *out);

/**
 * @brief SetRequiresRunRepairTimeReviser
 * @note Only available on [5.0.0+].
 * @param[in] flag Input flag.
 */
Result setsysSetRequiresRunRepairTimeReviser(bool flag);

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
 * @brief Gets the \ref SetSysHomeMenuScheme.
 * @note Only available on [9.0.0+].
 * @param[out] out \ref SetSysHomeMenuScheme
 */
Result setsysGetHomeMenuScheme(SetSysHomeMenuScheme *out);

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
