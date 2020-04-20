/**
 * @file hidsys.h
 * @brief hid:sys service IPC wrapper.
 * @author exelix, yellows8
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/hid.h"
#include "../sf/service.h"

/// ButtonConfig. Selects what button to map to.
typedef enum {
    HidcfgButtonConfig_A        = 0,          ///< A
    HidcfgButtonConfig_B        = 1,          ///< B
    HidcfgButtonConfig_X        = 2,          ///< X
    HidcfgButtonConfig_Y        = 3,          ///< Y
    HidcfgButtonConfig_LStick   = 4,          ///< Left Stick Button
    HidcfgButtonConfig_RStick   = 5,          ///< Right Stick Button
    HidcfgButtonConfig_L        = 6,          ///< L
    HidcfgButtonConfig_R        = 7,          ///< R
    HidcfgButtonConfig_ZL       = 8,          ///< ZL
    HidcfgButtonConfig_ZR       = 9,          ///< ZR
    HidcfgButtonConfig_Minus    = 10,         ///< Minus
    HidcfgButtonConfig_Plus     = 11,         ///< Plus
    HidcfgButtonConfig_DLeft    = 12,         ///< DLeft
    HidcfgButtonConfig_DUp      = 13,         ///< DUp
    HidcfgButtonConfig_DRight   = 14,         ///< DRight
    HidcfgButtonConfig_DDown    = 15,         ///< DDown
    HidcfgButtonConfig_SL_Left  = 16,         ///< SL on Left controller.
    HidcfgButtonConfig_SR_Left  = 17,         ///< SR on Left controller.
    HidcfgButtonConfig_SL_Right = 18,         ///< SL on Right controller.
    HidcfgButtonConfig_SR_Right = 19,         ///< SR on Right controller.
    HidcfgButtonConfig_HOME     = 20,         ///< HOME
    HidcfgButtonConfig_Capture  = 21,         ///< Capture
    HidcfgButtonConfig_Disabled = 22,         ///< Disabled
} HidcfgButtonConfig;

/// Mini Cycle struct for \ref HidsysNotificationLedPattern.
typedef struct {
    u8 ledIntensity;        ///< Mini Cycle X LED Intensity.
    u8 transitionSteps;     ///< Fading Transition Steps to Mini Cycle X (Uses PWM). Value 0x0: Instant. Each step duration is based on HidsysNotificationLedPattern::baseMiniCycleDuration.
    u8 finalStepDuration;   ///< Final Step Duration Multiplier of Mini Cycle X. Value 0x0: 12.5ms, 0x1 - xF: 1x - 15x. Value is a Multiplier of HidsysNotificationLedPattern::baseMiniCycleDuration.
    u8 pad;
} HidsysNotificationLedPatternCycle;

/// Structure for \ref hidsysSetNotificationLedPattern.
/// See also: https://switchbrew.org/wiki/HID_services#NotificationLedPattern
/// Only the low 4bits of each used byte in this struct is used.
typedef struct {
    u8 baseMiniCycleDuration;                           ///< Mini Cycle Base Duration. Value 0x1-0xF: 12.5ms - 187.5ms. Value 0x0 = 0ms/OFF.
    u8 totalMiniCycles;                                 ///< Number of Mini Cycles + 1. Value 0x0-0xF: 1 - 16 mini cycles.
    u8 totalFullCycles;                                 ///< Number of Full Cycles. Value 0x1-0xF: 1 - 15 full cycles. Value 0x0 is repeat forever, but if baseMiniCycleDuration is set to 0x0, it does the 1st Mini Cycle with a 12.5ms step duration and then the LED stays on with startIntensity.
    u8 startIntensity;                                  ///< LED Start Intensity. Value 0x0=0% - 0xF=100%.

    HidsysNotificationLedPatternCycle miniCycles[16];   ///< Mini Cycles

    u8 unk_x44[0x2];                                    ///< Unknown
    u8 pad_x46[0x2];                                    ///< Padding
} HidsysNotificationLedPattern;

/// ButtonConfigEmbedded
typedef struct {
    u8 unk_x0[0x2C8];
} HidsysButtonConfigEmbedded;

/// ButtonConfigFull
typedef struct {
    u8 unk_x0[0x2C8];
} HidsysButtonConfigFull;

/// ButtonConfigLeft
typedef struct {
    u8 unk_x0[0x1C8];
} HidsysButtonConfigLeft;

/// ButtonConfigRight
typedef struct {
    u8 unk_x0[0x1A0];
} HidsysButtonConfigRight;

/// JoystickConfig
typedef struct {
    u32 unk_x0;                                        ///< Orientation. 0 = default, 1 = enabled for Left, 2 = enabled for Right.
    u8 stick_change;                                   ///< StickChange
    u8 pad[3];                                         ///< Padding
} HidcfgJoystickConfig;

/// ButtonConfigEmbedded
typedef struct {
    u32 button_config[17];                             ///< \ref HidcfgButtonConfig, for the following buttons: DLeft, DUp, DRight, DDown, A, B, X, Y, LStick, RStick, L, R, ZL, ZR, Minus, Plus, Capture.
    HidcfgJoystickConfig joystick_config[2];           ///< \ref HidcfgJoystickConfig, for the left and right stick.
} HidcfgButtonConfigEmbedded;

/// ButtonConfigFull
typedef struct {
    u32 button_config[17];                             ///< \ref HidcfgButtonConfig, for the following buttons: DLeft, DUp, DRight, DDown, A, B, X, Y, LStick, RStick, L, R, ZL, ZR, Minus, Plus, Capture.
    HidcfgJoystickConfig joystick_config[2];           ///< \ref HidcfgJoystickConfig, for the left and right stick.
} HidcfgButtonConfigFull;

/// ButtonConfigLeft
typedef struct {
    u32 button_config[11];                             ///< \ref HidcfgButtonConfig, for the following buttons: DLeft, DUp, DRight, DDown, LStick, L, ZL, Minus, SL_Left, SR_Left, Capture.
    HidcfgJoystickConfig joystick_config;              ///< \ref HidcfgJoystickConfig
} HidcfgButtonConfigLeft;

/// ButtonConfigRight
typedef struct {
    u32 button_config[10];                             ///< \ref HidcfgButtonConfig, for the following buttons: A, B, X, Y, RStick, R, ZR, Plus, SL_Right, SR_Right.
    HidcfgJoystickConfig joystick_config;              ///< \ref HidcfgJoystickConfig
} HidcfgButtonConfigRight;

/// Initialize hidsys.
Result hidsysInitialize(void);

/// Exit hidsys.
void hidsysExit(void);

/// Gets the Service object for the actual hidsys service session.
Service* hidsysGetServiceSession(void);

/**
 * @brief Returns an event that fires when the home button is pressed, this will prevent the home menu from opening when the button is pressed.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
**/
Result hidsysAcquireHomeButtonEventHandle(Event* out_event);

/**
 * @brief Returns an event that fires when the sleep button is pressed.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
**/
Result hidsysAcquireSleepButtonEventHandle(Event* out_event);

/**
 * @brief Returns an event that fires when the capture button is pressed.
 * @note The Event must be closed by the user once finished with it.
 * @param[out] out_event Output Event with autoclear=false.
**/
Result hidsysAcquireCaptureButtonEventHandle(Event* out_event);

/**
 * @brief ActivateHomeButton
**/
Result hidsysActivateHomeButton(void);

/**
 * @brief ActivateSleepButton
**/
Result hidsysActivateSleepButton(void);

/**
 * @brief ActivateCaptureButton
**/
Result hidsysActivateCaptureButton(void);

/**
 * @brief Gets the SupportedNpadStyleSet for the CallerApplet. applet must be initialized in order to use this (uses \ref appletGetAppletResourceUserIdOfCallerApplet).
 * @note Only available on [6.0.0+].
 * @param[out] out \ref HidControllerType
 */
Result hidsysGetSupportedNpadStyleSetOfCallerApplet(HidControllerType *out);

/**
 * @brief Gets the UniquePadIds for the specified controller.
 * @note Only available on [3.0.0+].
 * @param id Controller ID. Must not be CONTROLLER_P1_AUTO.
 * @param UniquePadIds Output array of UniquePadIds.
 * @param Max number of entries for the UniquePadIds array.
 * @param total_entries Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadsFromNpad(HidControllerID id, u64 *UniquePadIds, s32 count, s32 *total_entries);

/**
 * @brief EnableAppletToGetInput
 * @param[in] enable Input flag.
**/
Result hidsysEnableAppletToGetInput(bool enable);

/**
 * @brief Gets a list of all UniquePadIds.
 * @param UniquePadIds Output array of UniquePadIds.
 * @param Max number of entries for the UniquePadIds array.
 * @param total_entries Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadIds(u64 *UniquePadIds, s32 count, s32 *total_entries);

/**
 * @brief Gets the unique pad's serial number.
 * @note Only available on [5.0.0+].
 * @param UniquePadId UniquePadId for the controller.
 * @param serial Pointer to output the serial to. (The buffer size needs to be at least 0x11 bytes)
 */
Result hidsysGetUniquePadSerialNumber(u64 UniquePadId, char *serial);

/**
 * @brief Sets the HOME-button notification LED pattern, for the specified controller.
 * @note Generally this should only be used if \ref hidsysSetNotificationLedPatternWithTimeout is not usable.
 * @note Only available on [7.0.0+].
 * @param pattern \ref HidsysNotificationLedPattern
 * @param UniquePadId UniquePadId for the controller.
 */
Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, u64 UniquePadId);

/**
 * @brief Sets the HOME-button notification LED pattern, for the specified controller. The LED will automatically be disabled once the specified timeout occurs.
 * @note Only available on [9.0.0+], and with controllers which have the [9.0.0+] firmware installed.
 * @param[in] pattern \ref HidsysNotificationLedPattern
 * @param[in] UniquePadId UniquePadId for the controller.
 * @param[in] timeout Timeout in nanoseconds.
 */
Result hidsysSetNotificationLedPatternWithTimeout(const HidsysNotificationLedPattern *pattern, u64 UniquePadId, u64 timeout);

/**
 * @brief IsButtonConfigSupported
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigSupported(u64 unique_pad_id, bool *out);

/**
 * @brief DeleteButtonConfig
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 */
Result hidsysDeleteButtonConfig(u64 unique_pad_id);

/**
 * @brief SetButtonConfigEnabled
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] flag Input flag.
 */
Result hidsysSetButtonConfigEnabled(u64 unique_pad_id, bool flag);

/**
 * @brief IsButtonConfigEnabled
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigEnabled(u64 unique_pad_id, bool *out);

/**
 * @brief SetButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysSetButtonConfigEmbedded(u64 unique_pad_id, const HidsysButtonConfigEmbedded *config);

/**
 * @brief SetButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidsysButtonConfigFull
 */
Result hidsysSetButtonConfigFull(u64 unique_pad_id, const HidsysButtonConfigFull *config);

/**
 * @brief SetButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidsysButtonConfigLeft
 */
Result hidsysSetButtonConfigLeft(u64 unique_pad_id, const HidsysButtonConfigLeft *config);

/**
 * @brief SetButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidsysButtonConfigRight
 */
Result hidsysSetButtonConfigRight(u64 unique_pad_id, const HidsysButtonConfigRight *config);

/**
 * @brief GetButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysGetButtonConfigEmbedded(u64 unique_pad_id, HidsysButtonConfigEmbedded *config);

/**
 * @brief GetButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidsysButtonConfigFull
 */
Result hidsysGetButtonConfigFull(u64 unique_pad_id, HidsysButtonConfigFull *config);

/**
 * @brief GetButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidsysButtonConfigLeft
 */
Result hidsysGetButtonConfigLeft(u64 unique_pad_id, HidsysButtonConfigLeft *config);

/**
 * @brief GetButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidsysButtonConfigRight
 */
Result hidsysGetButtonConfigRight(u64 unique_pad_id, HidsysButtonConfigRight *config);

/**
 * @brief IsCustomButtonConfigSupported
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] out Output bool flag.
 */
Result hidsysIsCustomButtonConfigSupported(u64 unique_pad_id, bool *out);

/**
 * @brief IsDefaultButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 * @param[out] out Output bool flag.
 */
Result hidsysIsDefaultButtonConfigEmbedded(const HidcfgButtonConfigEmbedded *config, bool *out);

/**
 * @brief IsDefaultButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] config \ref HidcfgButtonConfigFull
 * @param[out] out Output bool flag.
 */
Result hidsysIsDefaultButtonConfigFull(const HidcfgButtonConfigFull *config, bool *out);

/**
 * @brief IsDefaultButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] config \ref HidcfgButtonConfigLeft
 * @param[out] out Output bool flag.
 */
Result hidsysIsDefaultButtonConfigLeft(const HidcfgButtonConfigLeft *config, bool *out);

/**
 * @brief IsDefaultButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] config \ref HidcfgButtonConfigRight
 * @param[out] out Output bool flag.
 */
Result hidsysIsDefaultButtonConfigRight(const HidcfgButtonConfigRight *config, bool *out);

/**
 * @brief IsButtonConfigStorageEmbeddedEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageEmbeddedEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageFullEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageFullEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageLeftEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageLeftEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageRightEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageRightEmpty(s32 index, bool *out);

/**
 * @brief GetButtonConfigStorageEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysGetButtonConfigStorageEmbedded(s32 index, HidcfgButtonConfigEmbedded *config);

/**
 * @brief GetButtonConfigStorageFull
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] config \ref HidcfgButtonConfigFull
 */
Result hidsysGetButtonConfigStorageFull(s32 index, HidcfgButtonConfigFull *config);

/**
 * @brief GetButtonConfigStorageLeft
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] config \ref HidcfgButtonConfigLeft
 */
Result hidsysGetButtonConfigStorageLeft(s32 index, HidcfgButtonConfigLeft *config);

/**
 * @brief GetButtonConfigStorageRight
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[out] config \ref HidcfgButtonConfigRight
 */
Result hidsysGetButtonConfigStorageRight(s32 index, HidcfgButtonConfigRight *config);

/**
 * @brief SetButtonConfigStorageEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysSetButtonConfigStorageEmbedded(s32 index, const HidcfgButtonConfigEmbedded *config);

/**
 * @brief SetButtonConfigStorageFull
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[in] config \ref HidcfgButtonConfigFull
 */
Result hidsysSetButtonConfigStorageFull(s32 index, const HidcfgButtonConfigFull *config);

/**
 * @brief SetButtonConfigStorageLeft
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[in] config \ref HidcfgButtonConfigLeft
 */
Result hidsysSetButtonConfigStorageLeft(s32 index, const HidcfgButtonConfigLeft *config);

/**
 * @brief SetButtonConfigStorageRight
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 * @param[in] config \ref HidcfgButtonConfigRight
 */
Result hidsysSetButtonConfigStorageRight(s32 index, const HidcfgButtonConfigRight *config);

/**
 * @brief DeleteButtonConfigStorageEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 */
Result hidsysDeleteButtonConfigStorageEmbedded(s32 index);

/**
 * @brief DeleteButtonConfigStorageFull
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 */
Result hidsysDeleteButtonConfigStorageFull(s32 index);

/**
 * @brief DeleteButtonConfigStorageLeft
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 */
Result hidsysDeleteButtonConfigStorageLeft(s32 index);

/**
 * @brief DeleteButtonConfigStorageRight
 * @note Only available on [10.0.0+].
 * @param[in] index Array index for an array which contains a total of 5 entries.
 */
Result hidsysDeleteButtonConfigStorageRight(s32 index);

/**
 * @brief IsUsingCustomButtonConfig
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] out Output bool flag.
 */
Result hidsysIsUsingCustomButtonConfig(u64 unique_pad_id, bool *out);

/**
 * @brief IsAnyCustomButtonConfigEnabled
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] out Output bool flag.
 */
Result hidsysIsAnyCustomButtonConfigEnabled(u64 unique_pad_id, bool *out);

/**
 * @brief SetAllCustomButtonConfigEnabled
 * @note Only available on [10.0.0+].
 * @param[in] AppletResourceUserId AppletResourceUserId
 * @param[in] flag Input bool flag.
 */
Result hidsysSetAllCustomButtonConfigEnabled(u64 AppletResourceUserId, bool flag);

/**
 * @brief SetAllDefaultButtonConfig
 * @note Only available on [10.0.0+].
 */
Result hidsysSetAllDefaultButtonConfig(void);

/**
 * @brief SetHidButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysSetHidButtonConfigEmbedded(u64 unique_pad_id, const HidcfgButtonConfigEmbedded *config);

/**
 * @brief SetHidButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidcfgButtonConfigFull
 */
Result hidsysSetHidButtonConfigFull(u64 unique_pad_id, const HidcfgButtonConfigFull *config);

/**
 * @brief SetHidButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidcfgButtonConfigLeft
 */
Result hidsysSetHidButtonConfigLeft(u64 unique_pad_id, const HidcfgButtonConfigLeft *config);

/**
 * @brief SetHidButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[in] config \ref HidcfgButtonConfigRight
 */
Result hidsysSetHidButtonConfigRight(u64 unique_pad_id, const HidcfgButtonConfigRight *config);

/**
 * @brief GetHidButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysGetHidButtonConfigEmbedded(u64 unique_pad_id, HidcfgButtonConfigEmbedded *config);

/**
 * @brief GetHidButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidcfgButtonConfigFull
 */
Result hidsysGetHidButtonConfigFull(u64 unique_pad_id, HidcfgButtonConfigFull *config);

/**
 * @brief GetHidButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidcfgButtonConfigLeft
 */
Result hidsysGetHidButtonConfigLeft(u64 unique_pad_id, HidcfgButtonConfigLeft *config);

/**
 * @brief GetHidButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id UniquePadId for the controller.
 * @param[out] config \ref HidcfgButtonConfigRight
 */
Result hidsysGetHidButtonConfigRight(u64 unique_pad_id, HidcfgButtonConfigRight *config);

