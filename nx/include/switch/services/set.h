/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#include "../result.h"

#define SET_MAX_NAME_SIZE 0x30

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
Result setGetRegionCode(s32 *RegionCode);

Result setsysInitialize(void);
void setsysExit(void);

/// Gets the current system theme.
Result setsysGetColorSetId(ColorSetId* out);

/**
 * @brief Gets the size of a settings item value.
 * @param out Pointer to output the size to.
 */
Result setsysGetSettingsItemValueSize(const char *name, const char *item_key, u64 *size_out);

/**
 * @brief Gets the value of a settings item value.
 * @param out Pointer to output the value to.
 */
Result setsysGetSettingsItemValue(const char *name, const char *item_key, void *value_out, size_t value_out_size);

/**
 * @brief Gets the system's serial number.
 * @param serial Pointer to output the serial to. (The buffer size needs to be at least 0x19 bytes)
 */
Result setsysGetSerialNumber(char *serial);

/**
 * @brief Gets the lockscreen status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetLockScreenFlag(bool *out);

/**
 * @brief Gets the console information upload status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetConsoleInformationUploadFlag(bool *out);

/**
 * @brief Gets the automatic application download status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetAutomaticApplicationDownloadFlag(bool *out);

/**
 * @brief Gets the NFC status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetNfcEnableFlag(bool *out);

/**
 * @brief Gets the wireless LAN status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetWirelessLanEnableFlag(bool *out);

/**
 * @brief Gets the bluetooth status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetBluetoothEnableFlag(bool *out);