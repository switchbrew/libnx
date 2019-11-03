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
 * @brief Gets the \ref SetSysHomeMenuScheme.
 * @note Only available on [9.0.0+].
 * @param[out] out \ref SetSysHomeMenuScheme
 */
Result setsysGetHomeMenuScheme(SetSysHomeMenuScheme *out);

/**
 * @brief Gets the \ref SetSysPlatformRegion.
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
