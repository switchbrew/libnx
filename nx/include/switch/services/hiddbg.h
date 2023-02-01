/**
 * @file hiddbg.h
 * @brief hid:dbg service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../services/hid.h"
#include "../services/hidsys.h"
#include "../sf/service.h"

/// HiddbgNpadButton. For the remaining buttons, see \ref HidNpadButton.
typedef enum {
    HiddbgNpadButton_Home    = BIT(18),         ///< HOME button
    HiddbgNpadButton_Capture = BIT(19),         ///< Capture button
} HiddbgNpadButton;

/// HdlsAttribute
typedef enum {
    HiddbgHdlsAttribute_HasVirtualSixAxisSensorAcceleration      =    BIT(0),      ///< HasVirtualSixAxisSensorAcceleration
    HiddbgHdlsAttribute_HasVirtualSixAxisSensorAngle             =    BIT(1),      ///< HasVirtualSixAxisSensorAngle
} HiddbgHdlsAttribute;

/// State for overriding \ref HidDebugPadState.
typedef struct {
    u32 attributes;                             ///< Bitfield of \ref HidDebugPadAttribute.
    u32 buttons;                                ///< Bitfield of \ref HidDebugPadButton.
    HidAnalogStickState analog_stick_l;         ///< AnalogStickL
    HidAnalogStickState analog_stick_r;         ///< AnalogStickR
} HiddbgDebugPadAutoPilotState;

/// State for overriding \ref HidMouseState.
typedef struct {
    s32 x;                                      ///< X
    s32 y;                                      ///< Y
    s32 delta_x;                                ///< DeltaX
    s32 delta_y;                                ///< DeltaY
    s32 wheel_delta;                            ///< WheelDelta
    u32 buttons;                                ///< Bitfield of \ref HidMouseButton.
    u32 attributes;                             ///< Bitfield of \ref HidMouseAttribute.
} HiddbgMouseAutoPilotState;

/// State for overriding \ref HidKeyboardState.
typedef struct {
    u64 modifiers;                              ///< Bitfield of \ref HidKeyboardModifier.
    u64 keys[4];
} HiddbgKeyboardAutoPilotState;

/// State for overriding SleepButtonState.
typedef struct {
    u64 buttons;                                ///< Bitfield of buttons, only bit0 is used.
} HiddbgSleepButtonAutoPilotState;

/// HdlsHandle
typedef struct {
    u64 handle;               ///< Handle
} HiddbgHdlsHandle;

/// HdlsSessionId, returned by \ref hiddbgAttachHdlsWorkBuffer.
typedef struct {
    u64 id;                   ///< Id
} HiddbgHdlsSessionId;

/// HdlsDeviceInfo, for [7.0.0-8.1.0].
typedef struct {
    u32 deviceTypeInternal;   ///< Only one bit can be set. BIT(N*4+0) = Pro-Controller, BIT(N*4+1) = Joy-Con Left, BIT(N*4+2) = Joy-Con Right, BIT(N*4+3) = invalid. Where N is 0-1. BIT(8-10) = Pro-Controller, BIT(11) = Famicom-Controller, BIT(12) = Famicom-Controller II with microphone, BIT(13) = NES-Controller(DeviceType=0x200), BIT(14) = NES-Controller(DeviceType=0x400), BIT(15-16) = invalid, BIT(17) = unknown(DeviceType=0x8000), BIT(18-20) = invalid, BIT(21-23) = unknown(DeviceType=0x80000000).
    u32 singleColorBody;      ///< RGBA Single Body Color.
    u32 singleColorButtons;   ///< RGBA Single Buttons Color.
    u8 npadInterfaceType;     ///< \ref HidNpadInterfaceType. Additional type field used with the above type field (only applies to type bit0-bit2 and bit21), if the value doesn't match one of the following a default is used. Type Pro-Controller: value 0x3 indicates that the controller is connected via USB. Type BIT(21): value 0x3 = unknown. When value is 0x2, state is merged with an existing controller (when the type value is compatible with this). Otherwise, it's a dedicated controller.
    u8 pad[0x3];              ///< Padding.
} HiddbgHdlsDeviceInfoV7;

/// HdlsDeviceInfo, for [9.0.0+]. Converted to/from \ref HiddbgHdlsDeviceInfoV7 on prior sysvers.
typedef struct {
    u8 deviceType;            ///< \ref HidDeviceType
    u8 npadInterfaceType;     ///< \ref HidNpadInterfaceType. Additional type field used with the above type field (only applies to ::HidDeviceType_JoyRight1, ::HidDeviceType_JoyLeft2, ::HidDeviceType_FullKey3, and ::HidDeviceType_System19), if the value doesn't match one of the following a default is used. ::HidDeviceType_FullKey3: ::HidNpadInterfaceType_USB indicates that the controller is connected via USB. :::HidDeviceType_System19: ::HidNpadInterfaceType_USB = unknown. When value is ::HidNpadInterfaceType_Rail, state is merged with an existing controller (with ::HidDeviceType_JoyRight1 / ::HidDeviceType_JoyLeft2). Otherwise, it's a dedicated controller.
    u8 pad[0x2];              ///< Padding.
    u32 singleColorBody;      ///< RGBA Single Body Color.
    u32 singleColorButtons;   ///< RGBA Single Buttons Color.
    u32 colorLeftGrip;        ///< [9.0.0+] RGBA Left Grip Color.
    u32 colorRightGrip;       ///< [9.0.0+] RGBA Right Grip Color.
} HiddbgHdlsDeviceInfo;

/// HdlsState, for [7.0.0-8.1.0].
typedef struct {
    u8 is_powered;                                        ///< IsPowered for the main PowerInfo, see \ref HidNpadSystemProperties.
    u8 flags;                                             ///< ORRed with IsPowered to set the value of the first byte for \ref HidNpadSystemProperties. For example, value 1 here will set IsCharging for the main PowerInfo.
    u8 unk_x2[0x6];                                       ///< Unknown
    u32 battery_level;                                    ///< BatteryLevel for the main PowerInfo, see \ref HidPowerInfo.
    u32 buttons;                                          ///< See \ref HiddbgNpadButton.
    HidAnalogStickState analog_stick_l;                   ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                   ///< AnalogStickR
    u8 indicator;                                         ///< Indicator. Unused for input. Set with output from \ref hiddbgDumpHdlsStates. Not set by \ref hiddbgGetAbstractedPadsState.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsStateV7;

/// HdlsState, for [9.0.0-11.0.1].
typedef struct {
    u32 battery_level;                                    ///< BatteryLevel for the main PowerInfo, see \ref HidPowerInfo.
    u32 flags;                                            ///< Used to set the main PowerInfo for \ref HidNpadSystemProperties. BIT(0) -> IsPowered, BIT(1) -> IsCharging.
    u64 buttons;                                          ///< See \ref HiddbgNpadButton. [9.0.0+] Masked with 0xfffffffff00fffff.
    HidAnalogStickState analog_stick_l;                   ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                   ///< AnalogStickR
    u8 indicator;                                         ///< Indicator. Unused for input. Set with output from \ref hiddbgDumpHdlsStates.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsStateV9;

/// HdlsState, for [12.0.0+].
typedef struct {
    u32 battery_level;                                    ///< BatteryLevel for the main PowerInfo, see \ref HidPowerInfo.
    u32 flags;                                            ///< Used to set the main PowerInfo for \ref HidNpadSystemProperties. BIT(0) -> IsPowered, BIT(1) -> IsCharging.
    u64 buttons;                                          ///< See \ref HiddbgNpadButton. [9.0.0+] Masked with 0xfffffffff00fffff.
    HidAnalogStickState analog_stick_l;                   ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                   ///< AnalogStickR
    HidVector six_axis_sensor_acceleration;               ///< VirtualSixAxisSensorAcceleration
    HidVector six_axis_sensor_angle;                      ///< VirtualSixAxisSensorAngle
    u32 attribute;                                        ///< Bitfield of \ref HiddbgHdlsAttribute.
    u8 indicator;                                         ///< Indicator. Unused for input.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsState;

/// HdlsNpadAssignmentEntry
typedef struct {
    HiddbgHdlsHandle handle;                    ///< \ref HiddbgHdlsHandle
    u32 unk_x8;                                 ///< Unknown
    u32 unk_xc;                                 ///< Unknown
    u64 unk_x10;                                ///< Unknown
    u8 unk_x18;                                 ///< Unknown
    u8 pad[0x7];                                ///< Padding
} HiddbgHdlsNpadAssignmentEntry;

/// HdlsNpadAssignment. Same controllers as \ref HiddbgHdlsStateList, with different entry data.
typedef struct {
    s32 total_entries;                               ///< Total entries for the below entries.
    u32 pad;                                         ///< Padding
    HiddbgHdlsNpadAssignmentEntry entries[0x10];     ///< \ref HiddbgHdlsNpadAssignmentEntry
} HiddbgHdlsNpadAssignment;

/// HdlsStateListEntryV7, for [7.0.0-8.1.0].
typedef struct {
    HiddbgHdlsHandle handle;                    ///< \ref HiddbgHdlsHandle
    HiddbgHdlsDeviceInfoV7 device;              ///< \ref HiddbgHdlsDeviceInfoV7. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    HiddbgHdlsStateV7 state;                    ///< \ref HiddbgHdlsStateV7
} HiddbgHdlsStateListEntryV7;

/// HdlsStateListV7, for [7.0.0-8.1.0]. This contains a list of all controllers, including non-virtual controllers.
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntryV7 entries[0x10];   ///< \ref HiddbgHdlsStateListEntryV7
} HiddbgHdlsStateListV7;

/// HdlsStateListEntry, for [9.0.0-11.0.1].
typedef struct {
    HiddbgHdlsHandle handle;                    ///< \ref HiddbgHdlsHandle
    HiddbgHdlsDeviceInfo device;                ///< \ref HiddbgHdlsDeviceInfo. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    alignas(8) HiddbgHdlsStateV9 state;         ///< \ref HiddbgHdlsStateV9
} HiddbgHdlsStateListEntryV9;

/// HdlsStateList, for [9.0.0-11.0.1].
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntryV9 entries[0x10];   ///< \ref HiddbgHdlsStateListEntryV9
} HiddbgHdlsStateListV9;

/// HdlsStateListEntry, for [12.0.0+].
typedef struct {
    HiddbgHdlsHandle handle;                    ///< \ref HiddbgHdlsHandle
    HiddbgHdlsDeviceInfo device;                ///< \ref HiddbgHdlsDeviceInfo. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    alignas(8) HiddbgHdlsState state;           ///< \ref HiddbgHdlsState
} HiddbgHdlsStateListEntry;

/// HdlsStateList, for [12.0.0+].
/// This contains a list of all controllers, including non-virtual controllers.
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntry entries[0x10];     ///< \ref HiddbgHdlsStateListEntry
} HiddbgHdlsStateList;

/// AbstractedPadHandle
typedef struct {
    u64 handle;                                 ///< Handle
} HiddbgAbstractedPadHandle;

/// AbstractedPadState
typedef struct {
    u32 type;                 ///< Type. Converted to HiddbgHdlsDeviceInfoV7::type internally by \ref hiddbgSetAutoPilotVirtualPadState. BIT(0) -> BIT(0), BIT(1) -> BIT(15), BIT(2-3) -> BIT(1-2), BIT(4-5) -> BIT(1-2), BIT(6) -> BIT(3). BIT(7-11) -> BIT(11-15), BIT(12-14) -> BIT(12-14), BIT(15) -> BIT(17), BIT(31) -> BIT(21).
    u8 flags;                ///< Flags. Only bit0 is used by \ref hiddbgSetAutoPilotVirtualPadState, when clear it will skip using the rest of the input and run \ref hiddbgUnsetAutoPilotVirtualPadState internally.
    u8 pad[0x3];              ///< Padding

    u32 singleColorBody;      ///< RGBA Single Body Color
    u32 singleColorButtons;   ///< RGBA Single Buttons Color
    u8 npadInterfaceType;     ///< See HiddbgHdlsDeviceInfo::npadInterfaceType.
    u8 pad2[0x3];             ///< Padding

    HiddbgHdlsStateV7 state;  ///< State

    u8 unused[0x60];          ///< Unused with \ref hiddbgSetAutoPilotVirtualPadState. Not set by \ref hiddbgGetAbstractedPadsState.
} HiddbgAbstractedPadState;

/// Initialize hiddbg.
Result hiddbgInitialize(void);

/// Exit hiddbg.
void hiddbgExit(void);

/// Gets the Service object for the actual hiddbg service session.
Service* hiddbgGetServiceSession(void);

/**
 * @brief SetDebugPadAutoPilotState
 * @param[in] state \ref HiddbgDebugPadAutoPilotState
 */
Result hiddbgSetDebugPadAutoPilotState(const HiddbgDebugPadAutoPilotState *state);

/**
 * @brief UnsetDebugPadAutoPilotState
 */
Result hiddbgUnsetDebugPadAutoPilotState(void);

/**
 * @brief SetTouchScreenAutoPilotState
 * @param[in] states Input array of \ref HiddbgMouseAutoPilotState.
 * @param[in] count Total entries in the states array. Max is 16.
 */
Result hiddbgSetTouchScreenAutoPilotState(const HidTouchState *states, s32 count);

/**
 * @brief UnsetTouchScreenAutoPilotState
 */
Result hiddbgUnsetTouchScreenAutoPilotState(void);

/**
 * @brief SetMouseAutoPilotState
 * @param[in] state \ref HiddbgMouseAutoPilotState
 */
Result hiddbgSetMouseAutoPilotState(const HiddbgMouseAutoPilotState *state);

/**
 * @brief UnsetMouseAutoPilotState
 */
Result hiddbgUnsetMouseAutoPilotState(void);

/**
 * @brief SetKeyboardAutoPilotState
 * @param[in] state \ref HiddbgKeyboardAutoPilotState
 */
Result hiddbgSetKeyboardAutoPilotState(const HiddbgKeyboardAutoPilotState *state);

/**
 * @brief UnsetKeyboardAutoPilotState
 */
Result hiddbgUnsetKeyboardAutoPilotState(void);

/**
 * @brief Deactivates the HomeButton.
 */
Result hiddbgDeactivateHomeButton(void);

/**
 * @brief SetSleepButtonAutoPilotState
 * @param[in] state \ref HiddbgSleepButtonAutoPilotState
 */
Result hiddbgSetSleepButtonAutoPilotState(const HiddbgSleepButtonAutoPilotState *state);

/**
 * @brief UnsetSleepButtonAutoPilotState
 */
Result hiddbgUnsetSleepButtonAutoPilotState(void);

/**
 * @brief Writes the input RGB colors to the spi-flash for the specified UniquePad (offset 0x6050 size 0x6).
 * @note Only available with [3.0.0+].
 * @param[in] colorBody RGB body color.
 * @param[in] colorButtons RGB buttons color.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 */
Result hiddbgUpdateControllerColor(u32 colorBody, u32 colorButtons, HidsysUniquePadId unique_pad_id);

/**
 * @brief Writes the input RGB colors followed by inval to the spi-flash for the specified UniquePad (offset 0x6050 size 0xD).
 * @note Only available with [5.0.0+].
 * @param[in] colorBody RGB body color.
 * @param[in] colorButtons RGB buttons color.
 * @param[in] colorLeftGrip RGB left grip color.
 * @param[in] colorRightGrip RGB right grip color.
 * @param[in] inval Input value.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 */
Result hiddbgUpdateDesignInfo(u32 colorBody, u32 colorButtons, u32 colorLeftGrip, u32 colorRightGrip, u8 inval, HidsysUniquePadId unique_pad_id);

/**
 * @brief Get the OperationEvent for the specified UniquePad.
 * @note The Event must be closed by the user once finished with it.
 * @note Only available with [6.0.0+].
 * @param[out] out_event Output Event.
 * @param[in] autoclear The autoclear for the Event.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
**/
Result hiddbgAcquireOperationEventHandle(Event* out_event, bool autoclear, HidsysUniquePadId unique_pad_id);

/**
 * @brief Reads spi-flash for the specified UniquePad.
 * @note This also uses \ref hiddbgAcquireOperationEventHandle to wait for the operation to finish, then \ref hiddbgGetOperationResult is used.
 * @note Only available with [6.0.0+].
 * @param[in] offset Offset in spi-flash.
 * @param[out] buffer Output buffer.
 * @param[in] size Output buffer size.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
**/
Result hiddbgReadSerialFlash(u32 offset, void* buffer, size_t size, HidsysUniquePadId unique_pad_id);

/**
 * @brief Writes spi-flash for the specified UniquePad.
 * @note This also uses \ref hiddbgAcquireOperationEventHandle to wait for the operation to finish, then \ref hiddbgGetOperationResult is used.
 * @note Only available with [6.0.0+].
 * @param[in] offset Offset in spi-flash.
 * @param[in] buffer Input buffer, must be 0x1000-byte aligned.
 * @param[in] tmem_size Size of the buffer, must be 0x1000-byte aligned.
 * @param[in] size Actual transfer size.
 * @param[in] unique_pad_id \ref HidsysUniquePadId
**/
Result hiddbgWriteSerialFlash(u32 offset, void* buffer, size_t tmem_size, size_t size, HidsysUniquePadId unique_pad_id);

/**
 * @brief Get the Result for the Operation and handles cleanup, for the specified UniquePad.
 * @note Only available with [6.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
**/
Result hiddbgGetOperationResult(HidsysUniquePadId unique_pad_id);

/**
 * @brief Gets the internal DeviceType for the specified controller.
 * @note Only available with [6.0.0+].
 * @param[in] unique_pad_id \ref HidsysUniquePadId
 * @param[out] out Pre-9.0.0 this is an u32, with [9.0.0+] it's an u8.
**/
Result hiddbgGetUniquePadDeviceTypeSetInternal(HidsysUniquePadId unique_pad_id, u32 *out);

/** @name AbstractedPad
 *  This is for virtual HID controllers. Only use this on pre-7.0.0, Hdls should be used otherwise.
 */
///@{

/**
 * @brief Gets a list of \ref HiddbgAbstractedPadHandle.
 * @note Only available with [5.0.0-8.1.0].
 * @param[out] handles Output array of \ref HiddbgAbstractedPadHandle.
 * @param[in] count Max number of entries for the handles array.
 * @param[out] total_out Total output entries.
 */
Result hiddbgGetAbstractedPadHandles(HiddbgAbstractedPadHandle *handles, s32 count, s32 *total_out);

/**
 * @brief Gets the state for the specified \ref HiddbgAbstractedPadHandle.
 * @note Only available with [5.0.0-8.1.0].
 * @param[in] handle \ref HiddbgAbstractedPadHandle
 * @param[out] state \ref HiddbgAbstractedPadState
 */
Result hiddbgGetAbstractedPadState(HiddbgAbstractedPadHandle handle, HiddbgAbstractedPadState *state);

/**
 * @brief Similar to \ref hiddbgGetAbstractedPadHandles except this also returns the state for each pad in output array states.
 * @note Only available with [5.0.0-8.1.0].
 * @param[out] handles Output array of \ref HiddbgAbstractedPadHandle.
 * @param[out] states Output array of \ref HiddbgAbstractedPadState.
 * @param[in] count Max number of entries for the handles/states arrays.
 * @param[out] total_out Total output entries.
 */
Result hiddbgGetAbstractedPadsState(HiddbgAbstractedPadHandle *handles, HiddbgAbstractedPadState *states, s32 count, s32 *total_out);

/**
 * @brief Sets AutoPilot state for the specified pad.
 * @note Only available with [5.0.0-8.1.0].
 * @param[in] AbstractedVirtualPadId This can be any unique value as long as it's within bounds. For example, 0-7 is usable.
 * @param[in] state \ref HiddbgAbstractedPadState
 */
Result hiddbgSetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId, const HiddbgAbstractedPadState *state);

/**
 * @brief Clears AutoPilot state for the specified pad set by \ref hiddbgSetAutoPilotVirtualPadState.
 * @note Only available with [5.0.0-8.1.0].
 * @param[in] AbstractedVirtualPadId Id from \ref hiddbgSetAutoPilotVirtualPadState.
 */
Result hiddbgUnsetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId);

/**
 * @brief Clears AutoPilot state for all pads set by \ref hiddbgSetAutoPilotVirtualPadState.
 */
Result hiddbgUnsetAllAutoPilotVirtualPadState(void);

///@}

/** @name Hdls
 *  This is for virtual HID controllers.
 */
///@{

/**
 * @brief Initialize Hdls.
 * @note Only available with [7.0.0+].
 * @param[out] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[in] buffer An existing buffer to be used as transfer memory.
 * @param[in] size Size of the supplied buffer.
 */
Result hiddbgAttachHdlsWorkBuffer(HiddbgHdlsSessionId *session_id, void *buffer, size_t size);

/**
 * @brief Exit Hdls, must be called at some point prior to \ref hiddbgExit.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 */
Result hiddbgReleaseHdlsWorkBuffer(HiddbgHdlsSessionId session_id);

/**
 * @brief Checks if the given device is still attached.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[in] handle \ref HiddbgHdlsHandle
 * @param[out] out Whether the device is attached.
 */
Result hiddbgIsHdlsVirtualDeviceAttached(HiddbgHdlsSessionId session_id, HiddbgHdlsHandle handle, bool *out);

/**
 * @brief Gets state for \ref HiddbgHdlsNpadAssignment.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[out] state \ref HiddbgHdlsNpadAssignment
 */
Result hiddbgDumpHdlsNpadAssignmentState(HiddbgHdlsSessionId session_id, HiddbgHdlsNpadAssignment *state);

/**
 * @brief Gets state for \ref HiddbgHdlsStateList.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[out] state \ref HiddbgHdlsStateList
 */
Result hiddbgDumpHdlsStates(HiddbgHdlsSessionId session_id, HiddbgHdlsStateList *state);

/**
 * @brief Sets state for \ref HiddbgHdlsNpadAssignment.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[in] state \ref HiddbgHdlsNpadAssignment
 * @param[in] flag Flag
 */
Result hiddbgApplyHdlsNpadAssignmentState(HiddbgHdlsSessionId session_id, const HiddbgHdlsNpadAssignment *state, bool flag);

/**
 * @brief Sets state for \ref HiddbgHdlsStateList.
 * @note The \ref HiddbgHdlsState will be applied for each \ref HiddbgHdlsHandle. If a \ref HiddbgHdlsHandle is not found, code similar to \ref hiddbgAttachHdlsVirtualDevice will run with the \ref HiddbgHdlsDeviceInfo, then it will continue with applying state with the new device.
 * @note Only available with [7.0.0+].
 * @param[in] session_id [13.0.0+] \ref HiddbgHdlsSessionId
 * @param[in] state \ref HiddbgHdlsStateList
 */
Result hiddbgApplyHdlsStateList(HiddbgHdlsSessionId session_id, const HiddbgHdlsStateList *state);

/**
 * @brief Attach a device with the input info.
 * @note Only available with [7.0.0+].
 * @param[out] handle \ref HiddbgHdlsHandle
 * @param[in] info \ref HiddbgHdlsDeviceInfo
 */
Result hiddbgAttachHdlsVirtualDevice(HiddbgHdlsHandle *handle, const HiddbgHdlsDeviceInfo *info);

/**
 * @brief Detach the specified device.
 * @note Only available with [7.0.0+].
 * @param[in] handle \ref HiddbgHdlsHandle
 */
Result hiddbgDetachHdlsVirtualDevice(HiddbgHdlsHandle handle);

/**
 * @brief Sets state for the specified device.
 * @note Only available with [7.0.0+].
 * @param[in] handle \ref HiddbgHdlsHandle
 * @param[in] state \ref HiddbgHdlsState
 */
Result hiddbgSetHdlsState(HiddbgHdlsHandle handle, const HiddbgHdlsState *state);

///@}

