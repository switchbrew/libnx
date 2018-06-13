/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#include "../result.h"

#define SET_MAX_NAME_SIZE 0x48

typedef enum {
    ColorSetId_Light=0,
    ColorSetId_Dark=1
} ColorSetId;

/// IDs for Language.
typedef enum
{
    SetLanguage_JA =    0,  ///< Japanese
    SetLanguage_ENUS =  1,  ///< US English ("AmericanEnglish")
    SetLanguage_FR =    2,  ///< French
    SetLanguage_DE =    3,  ///< German
    SetLanguage_IT =    4,  ///< Italian
    SetLanguage_ES =    5,  ///< Spanish
    SetLanguage_ZHCN =  6,  ///< Simplified Chinese ("Chinese")
    SetLanguage_KO =    7,  ///< Korean
    SetLanguage_NL =    8,  ///< Dutch
    SetLanguage_PT =    9,  ///< Portuguese
    SetLanguage_RU =    10, ///< Russian
    SetLanguage_ZHTW =  11, ///< Traditional Chinese ("Taiwanese")
    SetLanguage_ENGB =  12, ///< GB English ("BritishEnglish")
    SetLanguage_FRCA =  13, ///< CA French ("CanadianFrench")
    SetLanguage_ES419 = 14, ///< "LatinAmericanSpanish"
    SetLanguage_Total,      ///< Total languages supported by this enum.
} SetLanguage;

/// Region codes.
typedef enum {
    SetRegion_JPN = 0, ///< Japan
    SetRegion_USA = 1, ///< The Americas
    SetRegion_EUR = 2, ///< Europe
    SetRegion_AUS = 3, ///< Australia/New Zealand
} SetRegion;

/// Command IDs for setsysGetFlag/setsysSetFlag.
typedef enum {
    SetSysFlag_LockScreen = 7,
    SetSysFlag_ConsoleInformationUpload = 25,
    SetSysFlag_AutomaticApplicationDownload = 27,
    SetSysFlag_Quest = 47,
    SetSysFlag_Usb30Enable = 65,
    SetSysFlag_NfcEnable = 69,
    SetSysFlag_WirelessLanEnable = 73,
    SetSysFlag_BluetoothEnable = 88,
    SetSysFlag_AutoUpdateEnable = 95,
    SetSysFlag_BatteryPercentage = 99,
    SetSysFlag_ExternalRtcReset = 101,
    SetSysFlag_UsbFullKeyEnable = 103,
    SetSysFlag_BluetoothAfhEnable = 111,
    SetSysFlag_BluetoothBoostEnable = 113,
    SetSysFlag_InRepairProcessEnable = 115,
    SetSysFlag_HeadphoneVolumeUpdate = 117,
} SetSysFlag;

Result setInitialize(void);
void setExit(void);

/// Converts LanguageCode to Language.
Result setMakeLanguage(u64 LanguageCode, s32 *Language);

/// Converts Language to LanguageCode.
Result setMakeLanguageCode(s32 Language, u64 *LanguageCode);

/// Gets the current system LanguageCode.
/// Normally this should be used instead of \ref setGetLanguageCode.
/// LanguageCode is a string, see here: http://switchbrew.org/index.php?title=Settings_services#LanguageCode
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

Result setsysInitialize(void);
void setsysExit(void);

/// Gets the current system theme.
Result setsysGetColorSetId(ColorSetId *out);

/// Sets the current system theme.
Result setsysSetColorSetId(ColorSetId id);

/**
 * @brief Gets the size of a settings item value.
 * @param out Pointer to output the size to.
 */
Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out);

/**
 * @brief Gets the value of a settings item.
 * @param out Pointer to output the value to.
 */
Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size);

/**
 * @brief Gets the system's serial number.
 * @param serial Pointer to output the serial to. (The buffer size needs to be at least 0x19 bytes)
 */
Result setsysGetSerialNumber(char *serial);

/**
 * @brief Gets the status of the specified settings flag.
 * @param flag The specified settings flag.
 * @param out Output pointer for the status.
 */
Result setsysGetFlag(SetSysFlag flag, bool *out);

/**
 * @brief Enables/disables the specified settings flag.
 * @param flag The specified settings flag.
 * @param enable To enable/disable the flag.
 */
Result setsysSetFlag(SetSysFlag flag, bool enable);