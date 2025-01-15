/**
 * @file hidsys.h
 * @brief hid:sys service IPC wrapper.
 * @author exelix, yellows8, ndeadly
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../services/hid.h"
#include "../services/btdrv_types.h"
#include "../sf/service.h"

/// Selects what button to map to.
typedef enum {
    HidcfgDigitalButtonAssignment_A              = 0,          ///< A
    HidcfgDigitalButtonAssignment_B              = 1,          ///< B
    HidcfgDigitalButtonAssignment_X              = 2,          ///< X
    HidcfgDigitalButtonAssignment_Y              = 3,          ///< Y
    HidcfgDigitalButtonAssignment_StickL         = 4,          ///< Left Stick Button
    HidcfgDigitalButtonAssignment_StickR         = 5,          ///< Right Stick Button
    HidcfgDigitalButtonAssignment_L              = 6,          ///< L
    HidcfgDigitalButtonAssignment_R              = 7,          ///< R
    HidcfgDigitalButtonAssignment_ZL             = 8,          ///< ZL
    HidcfgDigitalButtonAssignment_ZR             = 9,          ///< ZR
    HidcfgDigitalButtonAssignment_Select         = 10,         ///< Select / Minus
    HidcfgDigitalButtonAssignment_Start          = 11,         ///< Start / Plus
    HidcfgDigitalButtonAssignment_Left           = 12,         ///< Left
    HidcfgDigitalButtonAssignment_Up             = 13,         ///< Up
    HidcfgDigitalButtonAssignment_Right          = 14,         ///< Right
    HidcfgDigitalButtonAssignment_Down           = 15,         ///< Down
    HidcfgDigitalButtonAssignment_LeftSL         = 16,         ///< SL on Left controller.
    HidcfgDigitalButtonAssignment_LeftSR         = 17,         ///< SR on Left controller.
    HidcfgDigitalButtonAssignment_RightSL        = 18,         ///< SL on Right controller.
    HidcfgDigitalButtonAssignment_RightSR        = 19,         ///< SR on Right controller.
    HidcfgDigitalButtonAssignment_HomeButton     = 20,         ///< HomeButton
    HidcfgDigitalButtonAssignment_CaptureButton  = 21,         ///< CaptureButton
    HidcfgDigitalButtonAssignment_Invalid        = 22,         ///< Invalid / Disabled
} HidcfgDigitalButtonAssignment;

/// AnalogStickRotation
typedef enum {
    HidcfgAnalogStickRotation_None               = 0,          ///< None
    HidcfgAnalogStickRotation_Clockwise90        = 1,          ///< Clockwise90
    HidcfgAnalogStickRotation_Anticlockwise90    = 2,          ///< Anticlockwise90
} HidcfgAnalogStickRotation;

/// UniquePadType
typedef enum {
    HidsysUniquePadType_Embedded                 = 0,          ///< Embedded
    HidsysUniquePadType_FullKeyController        = 1,          ///< FullKeyController
    HidsysUniquePadType_RightController          = 2,          ///< RightController
    HidsysUniquePadType_LeftController           = 3,          ///< LeftController
    HidsysUniquePadType_DebugPadController       = 4,          ///< DebugPadController
} HidsysUniquePadType;

/// UniquePadId for a controller.
typedef struct {
    u64 id;                                   ///< UniquePadId
} HidsysUniquePadId;

/// UniquePadSerialNumber
typedef struct {
    char serial_number[0x10];                 ///< SerialNumber
} HidsysUniquePadSerialNumber;

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

/// AnalogStickAssignment
typedef struct {
    u32 rotation;                                      ///< \ref HidcfgAnalogStickRotation
    u8 is_paired_stick_assigned;                       ///< IsPairedStickAssigned
    u8 reserved[3];                                    ///< Reserved
} HidcfgAnalogStickAssignment;

/// ButtonConfigEmbedded
typedef struct {
    u32 hardware_button_left;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonLeft
    u32 hardware_button_up;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonUp
    u32 hardware_button_right;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonRight
    u32 hardware_button_down;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonDown
    u32 hardware_button_a;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonA
    u32 hardware_button_b;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonB
    u32 hardware_button_x;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonX
    u32 hardware_button_y;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonY
    u32 hardware_button_stick_l;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickL
    u32 hardware_button_stick_r;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickR
    u32 hardware_button_l;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonL
    u32 hardware_button_r;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonR
    u32 hardware_button_zl;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZL
    u32 hardware_button_zr;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZR
    u32 hardware_button_select;                        ///< \ref HidcfgDigitalButtonAssignment HardwareButtonSelect
    u32 hardware_button_start;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStart
    u32 hardware_button_capture;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonCapture
    HidcfgAnalogStickAssignment hardware_stick_l;      ///< HardwareStickL
    HidcfgAnalogStickAssignment hardware_stick_r;      ///< HardwareStickR
} HidcfgButtonConfigEmbedded;

/// ButtonConfigFull
typedef struct {
    u32 hardware_button_left;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonLeft
    u32 hardware_button_up;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonUp
    u32 hardware_button_right;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonRight
    u32 hardware_button_down;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonDown
    u32 hardware_button_a;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonA
    u32 hardware_button_b;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonB
    u32 hardware_button_x;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonX
    u32 hardware_button_y;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonY
    u32 hardware_button_stick_l;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickL
    u32 hardware_button_stick_r;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickR
    u32 hardware_button_l;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonL
    u32 hardware_button_r;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonR
    u32 hardware_button_zl;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZL
    u32 hardware_button_zr;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZR
    u32 hardware_button_select;                        ///< \ref HidcfgDigitalButtonAssignment HardwareButtonSelect
    u32 hardware_button_start;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStart
    u32 hardware_button_capture;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonCapture
    HidcfgAnalogStickAssignment hardware_stick_l;      ///< HardwareStickL
    HidcfgAnalogStickAssignment hardware_stick_r;      ///< HardwareStickR
} HidcfgButtonConfigFull;

/// ButtonConfigLeft
typedef struct {
    u32 hardware_button_left;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonLeft
    u32 hardware_button_up;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonUp
    u32 hardware_button_right;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonRight
    u32 hardware_button_down;                          ///< \ref HidcfgDigitalButtonAssignment HardwareButtonDown
    u32 hardware_button_stick_l;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickL
    u32 hardware_button_l;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonL
    u32 hardware_button_zl;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZL
    u32 hardware_button_select;                        ///< \ref HidcfgDigitalButtonAssignment HardwareButtonSelect
    u32 hardware_button_left_sl;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonLeftSL
    u32 hardware_button_left_sr;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonLeftSR
    u32 hardware_button_capture;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonCapture
    HidcfgAnalogStickAssignment hardware_stick_l;      ///< HardwareStickL
} HidcfgButtonConfigLeft;

/// ButtonConfigRight
typedef struct {
    u32 hardware_button_a;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonA
    u32 hardware_button_b;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonB
    u32 hardware_button_x;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonX
    u32 hardware_button_y;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonY
    u32 hardware_button_stick_r;                       ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStickR
    u32 hardware_button_r;                             ///< \ref HidcfgDigitalButtonAssignment HardwareButtonR
    u32 hardware_button_zr;                            ///< \ref HidcfgDigitalButtonAssignment HardwareButtonZR
    u32 hardware_button_start;                         ///< \ref HidcfgDigitalButtonAssignment HardwareButtonStart
    u32 hardware_button_right_sl;                      ///< \ref HidcfgDigitalButtonAssignment HardwareButtonRightSL
    u32 hardware_button_right_sr;                      ///< \ref HidcfgDigitalButtonAssignment HardwareButtonRightSR
    HidcfgAnalogStickAssignment hardware_stick_r;      ///< HardwareStickR
} HidcfgButtonConfigRight;

/// StorageName
typedef struct {
    u8 name[0x81];                                     ///< UTF-8 NUL-terminated name string.
} HidcfgStorageName;

/// Initialize hidsys.
Result hidsysInitialize(void);

/// Exit hidsys.
void hidsysExit(void);

/// Gets the Service object for the actual hidsys service session.
Service* hidsysGetServiceSession(void);

/**
 * @brief SendKeyboardLockKeyEvent
 * @param[in] events Bitfield of \ref HidKeyboardLockKeyEvent.
 */
Result hidsysSendKeyboardLockKeyEvent(u32 events);

/**
 * @brief Gets an Event which is signaled when HidHomeButtonState is updated.
 * @note The Event must be closed by the user once finished with it.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
 * @param[out] out_event Output Event.
 * @param[in] Event autoclear.
**/
Result hidsysAcquireHomeButtonEventHandle(Event* out_event, bool autoclear);

/**
 * @brief Activates the HomeButton sharedmem.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
**/
Result hidsysActivateHomeButton(void);

/**
 * @brief Gets an Event which is signaled when HidSleepButtonState is updated.
 * @note The Event must be closed by the user once finished with it.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
 * @param[out] out_event Output Event.
 * @param[in] Event autoclear.
**/
Result hidsysAcquireSleepButtonEventHandle(Event* out_event, bool autoclear);

/**
 * @brief Activates the SleepButton sharedmem.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
**/
Result hidsysActivateSleepButton(void);

/**
 * @brief Gets an Event which is signaled when HidCaptureButtonState is updated.
 * @note The Event must be closed by the user once finished with it.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
 * @param[out] out_event Output Event.
 * @param[in] Event autoclear.
**/
Result hidsysAcquireCaptureButtonEventHandle(Event* out_event, bool autoclear);

/**
 * @brief Activates the CaptureButton sharedmem.
 * @note This generally shouldn't be used, since AM-sysmodule uses it internally.
**/
Result hidsysActivateCaptureButton(void);

/**
 * @brief Applies npad system common policy.
 */
Result hidsysApplyNpadSystemCommonPolicy(void);

/**
 * @brief Gets the npad id type of the last controller that was active.
 * @param[out] out of \ref HidNpadIdType.
 */
Result hidsysGetLastActiveNpad(u32 *out);

/**
 * @brief Gets the SupportedNpadStyleSet for the CallerApplet. applet must be initialized in order to use this (uses \ref appletGetAppletResourceUserIdOfCallerApplet).
 * @note Only available on [6.0.0+].
 * @param[out] out Bitmask of \ref HidNpadStyleTag.
 */
Result hidsysGetSupportedNpadStyleSetOfCallerApplet(u32 *out);

/**
 * @brief Gets the \ref HidNpadInterfaceType for the specified controller.
 * @note Only available on [10.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out \ref HidNpadInterfaceType
 */
Result hidsysGetNpadInterfaceType(HidNpadIdType id, u8 *out);

/**
 * @brief GetNpadLeftRightInterfaceType
 * @note Only available on [10.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out0 \ref HidNpadInterfaceType
 * @param[out] out1 \ref HidNpadInterfaceType
 */
Result hidsysGetNpadLeftRightInterfaceType(HidNpadIdType id, u8 *out0, u8 *out1);

/**
 * @brief HasBattery
 * @note Only available on [10.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out Output flag.
 */
Result hidsysHasBattery(HidNpadIdType id, bool *out);

/**
 * @brief HasLeftRightBattery
 * @note Only available on [10.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out0 Output flag.
 * @param[out] out1 Output flag.
 */
Result hidsysHasLeftRightBattery(HidNpadIdType id, bool *out0, bool *out1);

/**
 * @brief Gets the UniquePadIds for the specified controller.
 * @note Only available on [3.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] unique_pad_ids Output array of \ref HidsysUniquePadId.
 * @param[in] count Max number of entries for the unique_pad_ids array.
 * @param[out] total_out Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadsFromNpad(HidNpadIdType id, HidsysUniquePadId *unique_pad_ids, s32 count, s32 *total_out);

/**
 * @brief EnableAppletToGetInput
 * @param[in] enable Input flag.
**/
Result hidsysEnableAppletToGetInput(bool enable);

/**
 * @brief EnableHandheldHids
**/
Result hidsysEnableHandheldHids(void);

/**
 * @brief DisableHandheldHids
**/
Result hidsysDisableHandheldHids(void);

/**
 * @brief SetJoyConRailEnabled
 * @note Only available on [9.0.0+].
 * @param[in] enable Input flag.
**/
Result hidsysSetJoyConRailEnabled(bool enable);

/**
 * @brief IsJoyConRailEnabled
 * @note Only available on [9.0.0+].
 * @param[out] out Output flag.
**/
Result hidsysIsJoyConRailEnabled(bool *out);

/**
 * @brief IsHandheldHidsEnabled
 * @note Only available on [10.0.0+].
 * @param[out] out Output flag.
**/
Result hidsysIsHandheldHidsEnabled(bool *out);

/**
 * @brief IsJoyConAttachedOnAllRail
 * @note Only available on [11.0.0+].
 * @param[out] out Output flag.
**/
Result hidsysIsJoyConAttachedOnAllRail(bool *out);

/**
 * @brief IsInvertedControllerConnectedOnRail
 * @note Only available on [19.0.0+].
 * @param[out] out Output flag.
**/
Result hidsysIsInvertedControllerConnectedOnRail(bool *out);

/**
 * @brief AcquireUniquePadConnectionEventHandle
 * @param[out] out_event Output Event.
 */
Result hidsysAcquireUniquePadConnectionEventHandle(Event *out_event);

/**
 * @brief Gets a list of all UniquePadIds.
 * @param[out] unique_pad_ids Output array of \ref HidsysUniquePadId.
 * @param[in] count Max number of entries for the unique_pad_ids array.
 * @param[out] total_out Total output array entries. Optional, can be NULL.
 */
Result hidsysGetUniquePadIds(HidsysUniquePadId *unique_pad_ids, s32 count, s32 *total_out);

/**
 * @brief AcquireJoyDetachOnBluetoothOffEventHandle
 * @param[out] out_event Output Event.
 * @param[in] Event autoclear.
 */
Result hidsysAcquireJoyDetachOnBluetoothOffEventHandle(Event *out_event, bool autoclear);

/**
 * @brief GetUniquePadBluetoothAddress
 * @note Only available on [3.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] address \ref BtdrvAddress
 */
Result hidsysGetUniquePadBluetoothAddress(HidsysUniquePadId unique_pad_id, BtdrvAddress *address);

/**
 * @brief DisconnectUniquePad
 * @note Only available on [3.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 */
Result hidsysDisconnectUniquePad(HidsysUniquePadId unique_pad_id);

/**
 * @brief GetUniquePadType
 * @note Only available on [5.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] pad_type \ref HidsysUniquePadType
 */
Result hidsysGetUniquePadType(HidsysUniquePadId unique_pad_id, HidsysUniquePadType *pad_type);

/**
 * @brief GetUniquePadInterface
 * @note Only available on [5.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] interface \ref HidNpadInterfaceType
 */
Result hidsysGetUniquePadInterface(HidsysUniquePadId unique_pad_id, HidNpadInterfaceType *interface);

/**
 * @brief Gets the \ref HidsysUniquePadSerialNumber.
 * @note Only available on [5.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] serial \ref HidsysUniquePadSerialNumber
 */
Result hidsysGetUniquePadSerialNumber(HidsysUniquePadId unique_pad_id, HidsysUniquePadSerialNumber *serial);

/**
 * @brief GetUniquePadControllerNumber
 * @note Only available on [5.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] number Controller number.
 */
Result hidsysGetUniquePadControllerNumber(HidsysUniquePadId unique_pad_id, u64 *number);

/**
 * @brief Sets the HOME-button notification LED pattern, for the specified controller.
 * @note Generally this should only be used if \ref hidsysSetNotificationLedPatternWithTimeout is not usable.
 * @note Only available on [7.0.0+].
 * @param[in] pattern \ref HidsysNotificationLedPattern
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 */
Result hidsysSetNotificationLedPattern(const HidsysNotificationLedPattern *pattern, HidsysUniquePadId unique_pad_id);

/**
 * @brief Sets the HOME-button notification LED pattern, for the specified controller. The LED will automatically be disabled once the specified timeout occurs.
 * @note Only available on [9.0.0+], and with controllers which have the [9.0.0+] firmware installed.
 * @param[in] pattern \ref HidsysNotificationLedPattern
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] timeout Timeout in nanoseconds.
 */
Result hidsysSetNotificationLedPatternWithTimeout(const HidsysNotificationLedPattern *pattern, HidsysUniquePadId unique_pad_id, u64 timeout);

/**
 * @brief IsUsbFullKeyControllerEnabled
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result hidsysIsUsbFullKeyControllerEnabled(bool *out);

/**
 * @brief EnableUsbFullKeyController
 * @note Only available on [3.0.0+].
 * @param[in] flag Flag
 */
Result hidsysEnableUsbFullKeyController(bool flag);

/**
 * @brief IsUsbConnected
 * @note Only available on [3.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output flag.
 */
Result hidsysIsUsbConnected(HidsysUniquePadId unique_pad_id, bool *out);

/**
 * @brief GetTouchScreenDefaultConfiguration
 * @note Only available on [9.0.0+].
 * @param[out] touch_screen_configuration \ref HidTouchScreenConfigurationForNx.
 */
Result hidsysGetTouchScreenDefaultConfiguration(HidTouchScreenConfigurationForNx *touch_screen_configuration);

/**
 * @brief IsFirmwareUpdateNeededForNotification
 * @note Only available on [9.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output flag.
 */
Result hidsysIsFirmwareUpdateNeededForNotification(HidsysUniquePadId unique_pad_id, bool *out);

/**
 * @brief Legacy IsButtonConfigSupported.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysIsButtonConfigSupported instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output bool flag.
 */
Result hidsysLegacyIsButtonConfigSupported(HidsysUniquePadId unique_pad_id, bool *out);

/**
 * @brief IsButtonConfigSupported
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyIsButtonConfigSupported instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigSupported(BtdrvAddress addr, bool *out);

/**
 * @brief IsButtonConfigEmbeddedSupported
 * @note Only available on [11.0.0-17.0.1].
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigEmbeddedSupported(bool *out);

/**
 * @brief Legacy DeleteButtonConfig.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysDeleteButtonConfig instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 */
Result hidsysLegacyDeleteButtonConfig(HidsysUniquePadId unique_pad_id);

/**
 * @brief DeleteButtonConfig
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyDeleteButtonConfig instead.
 * @param[in] addr \ref BtdrvAddress
 */
Result hidsysDeleteButtonConfig(BtdrvAddress addr);

/**
 * @brief DeleteButtonConfigEmbedded
 * @note Only available on [11.0.0-17.0.1].
 */
Result hidsysDeleteButtonConfigEmbedded(void);

/**
 * @brief Legacy SetButtonConfigEnabled.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysSetButtonConfigEnabled instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] flag Input flag.
 */
Result hidsysLegacySetButtonConfigEnabled(HidsysUniquePadId unique_pad_id, bool flag);

/**
 * @brief SetButtonConfigEnabled
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacySetButtonConfigEnabled instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] flag Input flag.
 */
Result hidsysSetButtonConfigEnabled(BtdrvAddress addr, bool flag);

/**
 * @brief SetButtonConfigEmbeddedEnabled
 * @note Only available on [11.0.0-17.0.1].
 * @param[in] flag Input flag.
 */
Result hidsysSetButtonConfigEmbeddedEnabled(bool flag);

/**
 * @brief Legacy IsButtonConfigEnabled.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysIsButtonConfigEnabled instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output bool flag.
 */
Result hidsysLegacyIsButtonConfigEnabled(HidsysUniquePadId unique_pad_id, bool *out);

/**
 * @brief IsButtonConfigEnabled
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyIsButtonConfigEnabled instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] out Output bool flag.
 */
Result hidsysIsButtonConfigEnabled(BtdrvAddress addr, bool *out);

/**
 * @brief IsButtonConfigEmbeddedEnabled
 * @note Only available on [11.0.0-17.0.1].
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigEmbeddedEnabled(bool *out);

/**
 * @brief Legacy SetButtonConfigEmbedded.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysSetButtonConfigEmbedded instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysLegacySetButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigEmbedded *config);

/**
 * @brief SetButtonConfigEmbedded
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacySetButtonConfigEmbedded instead.
 * @param[in] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysSetButtonConfigEmbedded(const HidsysButtonConfigEmbedded *config);

/**
 * @brief Legacy SetButtonConfigFull.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysSetButtonConfigFull instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidsysButtonConfigFull
 */
Result hidsysLegacySetButtonConfigFull(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigFull *config);

/**
 * @brief SetButtonConfigFull
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacySetButtonConfigFull instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] config \ref HidsysButtonConfigFull
 */
Result hidsysSetButtonConfigFull(BtdrvAddress addr, const HidsysButtonConfigFull *config);

/**
 * @brief Legacy SetButtonConfigLeft.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysSetButtonConfigLeft instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidsysButtonConfigLeft
 */
Result hidsysLegacySetButtonConfigLeft(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigLeft *config);

/**
 * @brief SetButtonConfigLeft
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacySetButtonConfigLeft instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] config \ref HidsysButtonConfigLeft
 */
Result hidsysSetButtonConfigLeft(BtdrvAddress addr, const HidsysButtonConfigLeft *config);

/**
 * @brief Legacy SetButtonConfigRight.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysSetButtonConfigRight instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidsysButtonConfigRight
 */
Result hidsysLegacySetButtonConfigRight(HidsysUniquePadId unique_pad_id, const HidsysButtonConfigRight *config);

/**
 * @brief SetButtonConfigRight
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacySetButtonConfigRight instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[in] config \ref HidsysButtonConfigRight
 */
Result hidsysSetButtonConfigRight(BtdrvAddress addr, const HidsysButtonConfigRight *config);

/**
 * @brief Legacy GetButtonConfigEmbedded.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysGetButtonConfigEmbedded instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysLegacyGetButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, HidsysButtonConfigEmbedded *config);

/**
 * @brief GetButtonConfigEmbedded
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyGetButtonConfigEmbedded instead.
 * @param[out] config \ref HidsysButtonConfigEmbedded
 */
Result hidsysGetButtonConfigEmbedded(HidsysButtonConfigEmbedded *config);

/**
 * @brief Legacy GetButtonConfigFull.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysGetButtonConfigFull instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidsysButtonConfigFull
 */
Result hidsysLegacyGetButtonConfigFull(HidsysUniquePadId unique_pad_id, HidsysButtonConfigFull *config);

/**
 * @brief GetButtonConfigFull
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyGetButtonConfigFull instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[out] config \ref HidsysButtonConfigFull
 */
Result hidsysGetButtonConfigFull(BtdrvAddress addr, HidsysButtonConfigFull *config);

/**
 * @brief Legacy GetButtonConfigLeft.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysGetButtonConfigLeft instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidsysButtonConfigLeft
 */
Result hidsysLegacyGetButtonConfigLeft(HidsysUniquePadId unique_pad_id, HidsysButtonConfigLeft *config);

/**
 * @brief GetButtonConfigLeft
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyGetButtonConfigLeft instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[out] config \ref HidsysButtonConfigLeft
 */
Result hidsysGetButtonConfigLeft(BtdrvAddress addr, HidsysButtonConfigLeft *config);

/**
 * @brief Legacy GetButtonConfigRight.
 * @note Only available on [10.0.0-10.2.0]. On [11.0.0-17.0.1], use \ref hidsysGetButtonConfigRight instead.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidsysButtonConfigRight
 */
Result hidsysLegacyGetButtonConfigRight(HidsysUniquePadId unique_pad_id, HidsysButtonConfigRight *config);

/**
 * @brief GetButtonConfigRight
 * @note Only available on [11.0.0-17.0.1]. On [10.0.0-10.2.0], use \ref hidsysLegacyGetButtonConfigRight instead.
 * @param[in] addr \ref BtdrvAddress
 * @param[out] config \ref HidsysButtonConfigRight
 */
Result hidsysGetButtonConfigRight(BtdrvAddress addr, HidsysButtonConfigRight *config);

/**
 * @brief IsCustomButtonConfigSupported
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output bool flag.
 */
Result hidsysIsCustomButtonConfigSupported(HidsysUniquePadId unique_pad_id, bool *out);

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
 * @param[in] index Array index, should be 0-4.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageEmbeddedEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageFullEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageFullEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageLeftEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageLeftEmpty(s32 index, bool *out);

/**
 * @brief IsButtonConfigStorageRightEmpty
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] out Output bool flag.
 */
Result hidsysIsButtonConfigStorageRightEmpty(s32 index, bool *out);

/**
 * @brief GetButtonConfigStorageEmbeddedDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysGetButtonConfigStorageEmbeddedDeprecated(s32 index, HidcfgButtonConfigEmbedded *config);

/**
 * @brief GetButtonConfigStorageFullDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigFull
 */
Result hidsysGetButtonConfigStorageFullDeprecated(s32 index, HidcfgButtonConfigFull *config);

/**
 * @brief GetButtonConfigStorageLeftDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigLeft
 */
Result hidsysGetButtonConfigStorageLeftDeprecated(s32 index, HidcfgButtonConfigLeft *config);

/**
 * @brief GetButtonConfigStorageRightDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigRight
 */
Result hidsysGetButtonConfigStorageRightDeprecated(s32 index, HidcfgButtonConfigRight *config);

/**
 * @brief SetButtonConfigStorageEmbeddedDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysSetButtonConfigStorageEmbeddedDeprecated(s32 index, const HidcfgButtonConfigEmbedded *config);

/**
 * @brief SetButtonConfigStorageFullDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigFull
 */
Result hidsysSetButtonConfigStorageFullDeprecated(s32 index, const HidcfgButtonConfigFull *config);

/**
 * @brief SetButtonConfigStorageLeftDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigLeft
 */
Result hidsysSetButtonConfigStorageLeftDeprecated(s32 index, const HidcfgButtonConfigLeft *config);

/**
 * @brief SetButtonConfigStorageRightDeprecated
 * @note Only available on [10.0.0-12.1.0].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigRight
 */
Result hidsysSetButtonConfigStorageRightDeprecated(s32 index, const HidcfgButtonConfigRight *config);

/**
 * @brief DeleteButtonConfigStorageEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 */
Result hidsysDeleteButtonConfigStorageEmbedded(s32 index);

/**
 * @brief DeleteButtonConfigStorageFull
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 */
Result hidsysDeleteButtonConfigStorageFull(s32 index);

/**
 * @brief DeleteButtonConfigStorageLeft
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 */
Result hidsysDeleteButtonConfigStorageLeft(s32 index);

/**
 * @brief DeleteButtonConfigStorageRight
 * @note Only available on [10.0.0+].
 * @param[in] index Array index, should be 0-4.
 */
Result hidsysDeleteButtonConfigStorageRight(s32 index);

/**
 * @brief IsUsingCustomButtonConfig
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Output bool flag.
 */
Result hidsysIsUsingCustomButtonConfig(HidsysUniquePadId unique_pad_id, bool *out);

/**
 * @brief IsAnyCustomButtonConfigEnabled
 * @note Only available on [10.0.0+].
 * @param[out] out Output bool flag.
 */
Result hidsysIsAnyCustomButtonConfigEnabled(bool *out);

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
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysSetHidButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigEmbedded *config);

/**
 * @brief SetHidButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidcfgButtonConfigFull
 */
Result hidsysSetHidButtonConfigFull(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigFull *config);

/**
 * @brief SetHidButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidcfgButtonConfigLeft
 */
Result hidsysSetHidButtonConfigLeft(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigLeft *config);

/**
 * @brief SetHidButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[in] config \ref HidcfgButtonConfigRight
 */
Result hidsysSetHidButtonConfigRight(HidsysUniquePadId unique_pad_id, const HidcfgButtonConfigRight *config);

/**
 * @brief GetHidButtonConfigEmbedded
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidcfgButtonConfigEmbedded
 */
Result hidsysGetHidButtonConfigEmbedded(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigEmbedded *config);

/**
 * @brief GetHidButtonConfigFull
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidcfgButtonConfigFull
 */
Result hidsysGetHidButtonConfigFull(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigFull *config);

/**
 * @brief GetHidButtonConfigLeft
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidcfgButtonConfigLeft
 */
Result hidsysGetHidButtonConfigLeft(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigLeft *config);

/**
 * @brief GetHidButtonConfigRight
 * @note Only available on [10.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] config \ref HidcfgButtonConfigRight
 */
Result hidsysGetHidButtonConfigRight(HidsysUniquePadId unique_pad_id, HidcfgButtonConfigRight *config);

/**
 * @brief GetButtonConfigStorageEmbedded
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigEmbedded
 * @param[out] name \ref HidcfgStorageName
 */
Result hidsysGetButtonConfigStorageEmbedded(s32 index, HidcfgButtonConfigEmbedded *config, HidcfgStorageName *name);

/**
 * @brief GetButtonConfigStorageFull
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigFull
 * @param[out] name \ref HidcfgStorageName
 */
Result hidsysGetButtonConfigStorageFull(s32 index, HidcfgButtonConfigFull *config, HidcfgStorageName *name);

/**
 * @brief GetButtonConfigStorageLeft
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigLeft
 * @param[out] name \ref HidcfgStorageName
 */
Result hidsysGetButtonConfigStorageLeft(s32 index, HidcfgButtonConfigLeft *config, HidcfgStorageName *name);

/**
 * @brief GetButtonConfigStorageRight
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[out] config \ref HidcfgButtonConfigRight
 * @param[out] name \ref HidcfgStorageName
 */
Result hidsysGetButtonConfigStorageRight(s32 index, HidcfgButtonConfigRight *config, HidcfgStorageName *name);

/**
 * @brief SetButtonConfigStorageEmbedded
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigEmbedded
 * @param[in] name \ref HidcfgStorageName
 */
Result hidsysSetButtonConfigStorageEmbedded(s32 index, const HidcfgButtonConfigEmbedded *config, const HidcfgStorageName *name);

/**
 * @brief SetButtonConfigStorageFull
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigFull
 * @param[in] name \ref HidcfgStorageName
 */
Result hidsysSetButtonConfigStorageFull(s32 index, const HidcfgButtonConfigFull *config, const HidcfgStorageName *name);

/**
 * @brief SetButtonConfigStorageLeft
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigLeft
 * @param[in] name \ref HidcfgStorageName
 */
Result hidsysSetButtonConfigStorageLeft(s32 index, const HidcfgButtonConfigLeft *config, const HidcfgStorageName *name);

/**
 * @brief SetButtonConfigStorageRight
 * @note Only available on [11.0.0+].
 * @param[in] index Array index, should be 0-4.
 * @param[in] config \ref HidcfgButtonConfigRight
 * @param[in] name \ref HidcfgStorageName
 */
Result hidsysSetButtonConfigStorageRight(s32 index, const HidcfgButtonConfigRight *config, const HidcfgStorageName *name);
