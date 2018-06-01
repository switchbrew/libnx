/**
 * @file set.h
 * @brief Settings services IPC wrapper.
 * @author plutoo
 * @author yellows8
 * @copyright libnx Authors
 */
#include "../result.h"

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
 * @brief Sets the lockscreen status.
 * @param flag Whether to enable the lockscreen flag.
 */
Result setsysSetLockScreenFlag(bool flag);

/**
 * @brief Gets the console information upload status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetConsoleInformationUploadFlag(bool *out);

/**
 * @brief Sets the console information upload status.
 * @param flag Whether to enable the console information upload flag.
 */
Result setsysSetConsoleInformationUploadFlag(bool flag);

/**
 * @brief Gets the automatic application download status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetAutomaticApplicationDownloadFlag(bool *out);

/**
 * @brief Sets the automatic application download status.
 * @param flag Whether to enable the automatic application download flag.
 */
Result setsysSetAutomaticApplicationDownloadFlag(bool flag);

/**
 * @brief Gets the quest flag status (Determines if the deivce is a kiosk unit).
 * @param out Pointer to output the status to. 
 */
Result setsysGetQuestFlag(bool *out);

/**
 * @brief Sets the quest flag status.
 * @param flag Whether to enable the kiosk device flag.
 */
Result setsysSetQuestFlag(bool flag);

/**
 * @brief Gets the USB 30 status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetUsb30EnableFlag(bool *out);

/**
 * @brief Sets the USB 30 status.
 * @param flag Whether to enable the USB 30 flag.
 */
Result setsysSetUsb30EnableFlag(bool flag);

/**
 * @brief Gets the NFC status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetNfcEnableFlag(bool *out);

/**
 * @brief Sets the NFC status.
 * @param flag Whether to enable the NFC flag.
 */
Result setsysSetNfcEnableFlag(bool flag);

/**
 * @brief Gets the wireless LAN status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetWirelessLanEnableFlag(bool *out);

/**
 * @brief Sets the wireless LAN status.
 * @param flag Whether to enable the wireless LAN flag.
 */
Result setsysSetWirelessLanEnableFlag(bool flag);

/**
 * @brief Gets the bluetooth status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetBluetoothEnableFlag(bool *out);

/**
 * @brief Sets the Bluetooth status.
 * @param flag Whether to enable the bluetooth flag.
 */
Result setsysSetBluetoothEnableFlag(bool flag);

/**
 * @brief Gets the auto update status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetAutoUpdateEnableFlag(bool *out);

/**
 * @brief Sets the auto update status.
 * @param flag Whether to enable the auto update flag.
 */
Result setsysSetAutoUpdateEnableFlag(bool flag);

/**
 * @brief Gets the battery percentage status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetBatteryPercentageFlag(bool *out);

/**
 * @brief Sets the battery percentage status.
 * @param flag Whether to enable the battery percentage flag.
 */
Result setsysSetBatteryPercentageFlag(bool flag);

/**
 * @brief Gets the external RTC status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetExternalRtcResetFlag(bool *out);

/**
 * @brief Sets the external RTC status.
 * @param flag Whether to enable the external RTC flag.
 */
Result setsysSetExternalRtcResetFlag(bool flag);

/**
 * @brief Gets the USB full key status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetUsbFullKeyEnableFlag(bool *out);

/**
 * @brief Sets the USB full key status.
 * @param flag Whether to enable the USB full key flag.
 */
Result setsysSetUsbFullKeyEnableFlag(bool flag);

/**
 * @brief Gets the bluetooth AFH status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetBluetoothAfhEnableFlag(bool *out);

/**
 * @brief Sets the bluetooth AFH status.
 * @param flag Whether to enable the bluetooth AFH flag.
 */
Result setsysSetBluetoothAfhEnableFlag(bool flag);

/**
 * @brief Gets the bluetooth boost status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetBluetoothBoostEnableFlag(bool *out);

/**
 * @brief Sets the bluetooth boost status.
 * @param flag Whether to enable the bluetooth boost flag.
 */
Result setsysSetBluetoothBoostEnableFlag(bool flag);

/**
 * @brief Gets the repair process status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetInRepairProcessEnableFlag(bool *out);

/**
 * @brief Sets the repair process status.
 * @param flag Whether to enable the repair process flag.
 */
Result setsysSetInRepairProcessEnableFlag(bool flag);

/**
 * @brief Gets the headphone volume update status.
 * @param out Pointer to output the status to. 
 */
Result setsysGetHeadphoneVolumeUpdateFlag(bool *out);

/**
 * @brief Sets the headphone volume update status.
 * @param flag Whether to enable the headphone volume update flag.
 */
Result setsysSetHeadphoneVolumeUpdateFlag(bool flag);
