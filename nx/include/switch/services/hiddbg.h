/**
 * @file hiddbg.h
 * @brief hid:dbg service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../services/hid.h"
#include "../sf/service.h"

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
    u8 npadInterfaceType;     ///< \ref HidNpadInterfaceType. Additional type field used with the above type field (only applies to ::HidDeviceType_JoyRight1, ::HidDeviceType_JoyLeft2, ::HidDeviceType_FullKey3, and ::HidDeviceType_System19), if the value doesn't match one of the following a default is used. ::HidDeviceType_FullKey3: ::NpadInterfaceType_USB indicates that the controller is connected via USB. :::HidDeviceType_System19: ::NpadInterfaceType_USB = unknown. When value is ::NpadInterfaceType_Rail, state is merged with an existing controller (with ::HidDeviceType_JoyRight1 / ::HidDeviceType_JoyLeft2). Otherwise, it's a dedicated controller.
    u8 pad[0x2];              ///< Padding.
    u32 singleColorBody;      ///< RGBA Single Body Color.
    u32 singleColorButtons;   ///< RGBA Single Buttons Color.
    u32 colorLeftGrip;        ///< [9.0.0+] RGBA Left Grip Color.
    u32 colorRightGrip;       ///< [9.0.0+] RGBA Right Grip Color.
} HiddbgHdlsDeviceInfo;

/// HdlsState, for [7.0.0-8.1.0].
typedef struct {
    u8 powerConnected;                                    ///< powerConnected for the main PowerInfo, see \ref HidFlags.
    u8 flags;                                             ///< ORRed with powerConnected to set the value of the first byte for \ref HidFlags. For example, value 1 here will set isCharging for the main PowerInfo.
    u8 unk_x2[0x6];                                       ///< Unknown
    u32 batteryCharge;                                    ///< batteryCharge for the main PowerInfo, see \ref HidPowerInfo.
    u32 buttons;                                          ///< See \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];      ///< \ref JoystickPosition
    u8 unk_x20;                                           ///< Unused for input. Set with output from \ref hiddbgDumpHdlsStates. Not set by \ref hiddbgGetAbstractedPadsState.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsStateV7;

/// HdlsState, for [9.0.0+]. Converted to/from \ref HiddbgHdlsDeviceInfoV7 on prior sysvers.
typedef struct {
    u32 batteryCharge;                                    ///< batteryCharge for the main PowerInfo, see \ref HidPowerInfo.
    u32 flags;                                            ///< Used to set the main PowerInfo for \ref HidFlags. BIT(0) -> powerConnected, BIT(1) -> isCharging.
    u64 buttons;                                          ///< See \ref HidControllerKeys. [9.0.0+] Masked with 0xfffffffff00fffff.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];      ///< \ref JoystickPosition
    u8 unk_x20;                                           ///< Unused for input. Set with output from \ref hiddbgDumpHdlsStates.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsState;

/// HdlsNpadAssignmentEntry
typedef struct {
    u64 HdlsHandle;                             ///< HdlsHandle
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
    u64 HdlsHandle;                             ///< HdlsHandle
    HiddbgHdlsDeviceInfoV7 device;              ///< \ref HiddbgHdlsDeviceInfoV7. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    HiddbgHdlsStateV7 state;                      ///< \ref HiddbgHdlsState
} HiddbgHdlsStateListEntryV7;

/// HdlsStateListV7, for [7.0.0-8.1.0]. This contains a list of all controllers, including non-virtual controllers.
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntryV7 entries[0x10];   ///< \ref HiddbgHdlsStateListEntryV7
} HiddbgHdlsStateListV7;

/// HdlsStateListEntry, for [9.0.0+]. Converted to/from \ref HiddbgHdlsStateListEntryV7 on prior sysvers.
typedef struct {
    u64 HdlsHandle;                             ///< HdlsHandle
    HiddbgHdlsDeviceInfo device;                ///< \ref HiddbgHdlsDeviceInfo. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    alignas(8) HiddbgHdlsState state;           ///< \ref HiddbgHdlsState
} HiddbgHdlsStateListEntry;

/// HdlsStateList, for [9.0.0+]. Converted to/from \ref HiddbgHdlsStateListV7 on prior sysvers.
/// This contains a list of all controllers, including non-virtual controllers.
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntry entries[0x10];     ///< \ref HiddbgHdlsStateListEntry
} HiddbgHdlsStateList;

/// AbstractedPadState
typedef struct {
    u32 type;                 ///< Type. Converted to HiddbgHdlsDeviceInfoV7::type internally by \ref hiddbgSetAutoPilotVirtualPadState. BIT(0) -> BIT(0), BIT(1) -> BIT(15), BIT(2-3) -> BIT(1-2), BIT(4-5) -> BIT(1-2), BIT(6) -> BIT(3). BIT(7-11) -> BIT(11-15), BIT(12-14) -> BIT(12-14), BIT(15) -> BIT(17), BIT(31) -> BIT(21).
    u8 flags;                ///< Flags. Only bit0 is used by \ref hiddbgSetAutoPilotVirtualPadState: when clear it will skip using the rest of the input and run \ref hiddbgUnsetAutoPilotVirtualPadState internally.
    u8 pad[0x3];              ///< Padding

    u32 singleColorBody;      ///< RGBA Single Body Color
    u32 singleColorButtons;   ///< RGBA Single Buttons Color
    u8 npadInterfaceType;     ///< See HiddbgHdlsDeviceInfo::npadInterfaceType.
    u8 pad2[0x3];             ///< Padding

    HiddbgHdlsStateV7 state;    ///< State

    u8 unused[0x60];         ///< Unused with \ref hiddbgSetAutoPilotVirtualPadState. Not set by \ref hiddbgGetAbstractedPadsState.
} HiddbgAbstractedPadState;

/// Initialize hiddbg.
Result hiddbgInitialize(void);

/// Exit hiddbg.
void hiddbgExit(void);

/// Gets the Service object for the actual hiddbg service session.
Service* hiddbgGetServiceSession(void);

/// Writes the input RGB colors to the spi-flash for the specified controller (offset 0x6050 size 0x6). See hidsys.h for UniquePadId. Only available with [3.0.0+].
Result hiddbgUpdateControllerColor(u32 colorBody, u32 colorButtons, u64 UniquePadId);

/// Writes the input RGB colors followed by inval to the spi-flash for the specified controller (offset 0x6050 size 0xD). See hidsys.h for UniquePadId. Only available with [5.0.0+].
Result hiddbgUpdateDesignInfo(u32 colorBody, u32 colorButtons, u32 colorLeftGrip, u32 colorRightGrip, u8 inval, u64 UniquePadId);

/// Get the OperationEvent for the specified controller. See hidsys.h for UniquePadId.
/// The Event must be closed by the user once finished with it.
/// Only available with [6.0.0+].
Result hiddbgAcquireOperationEventHandle(Event* out_event, bool autoclear, u64 UniquePadId);

/// Reads spi-flash for the specified controller. See hidsys.h for UniquePadId.
/// This also uses \ref hiddbgAcquireOperationEventHandle to wait for the operation to finish, then \ref hiddbgGetOperationResult is used.
/// Only available with [6.0.0+].
Result hiddbgReadSerialFlash(u32 offset, void* buffer, size_t size, u64 UniquePadId);

/// Writes spi-flash for the specified controller. See hidsys.h for UniquePadId.
/// buffer and tmem_size must be page-aligned. size is the actual transfer size.
/// This also uses \ref hiddbgAcquireOperationEventHandle to wait for the operation to finish, then \ref hiddbgGetOperationResult is used.
/// Only available with [6.0.0+].
Result hiddbgWriteSerialFlash(u32 offset, void* buffer, size_t tmem_size, size_t size, u64 UniquePadId);

/// Get the Result for the Operation and handles cleanup, for the specified controller. See hidsys.h for UniquePadId.
/// Only available with [6.0.0+].
Result hiddbgGetOperationResult(u64 UniquePadId);

/// Gets the internal DeviceType for the specified controller. See hidsys.h for UniquePadId.
/// Only available with [6.0.0+].
/// Pre-9.0.0 the output is an u32, with [9.0.0+] it's an u8.
Result hiddbgGetUniquePadDeviceTypeSetInternal(u64 UniquePadId, u32 *out);

/// Gets a list of AbstractedPadHandles, where AbstractedPadHandles is the output array with max entries = count. total_entries is total entries written to the output array.
/// Only available with [5.0.0-8.1.0].
Result hiddbgGetAbstractedPadHandles(u64 *AbstractedPadHandles, s32 count, s32 *total_entries);

/// Gets the state for the specified AbstractedPadHandle.
/// Only available with [5.0.0-8.1.0].
Result hiddbgGetAbstractedPadState(u64 AbstractedPadHandle, HiddbgAbstractedPadState *state);

/// Similar to \ref hiddbgGetAbstractedPadHandles except this also returns the state for each pad in output array states.
/// Only available with [5.0.0-8.1.0].
Result hiddbgGetAbstractedPadsState(u64 *AbstractedPadHandles, HiddbgAbstractedPadState *states, s32 count, s32 *total_entries);

/// Sets AutoPilot state for the specified pad.
/// AbstractedVirtualPadId can be any unique value as long as it's within bounds. For example, 0-7 is usable.
/// Only available with [5.0.0-8.1.0].
Result hiddbgSetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId, const HiddbgAbstractedPadState *state);

/// Clears AutoPilot state for the specified pad set by \ref hiddbgSetAutoPilotVirtualPadState.
/// Only available with [5.0.0-8.1.0].
Result hiddbgUnsetAutoPilotVirtualPadState(s8 AbstractedVirtualPadId);

/// Clears AutoPilot state for all pads set by \ref hiddbgSetAutoPilotVirtualPadState.
Result hiddbgUnsetAllAutoPilotVirtualPadState(void);

/// Initialize Hdls. Hdls is for virtual HID controllers. Only available with [7.0.0+].
Result hiddbgAttachHdlsWorkBuffer(void);

/// Exit Hdls, must be called at some point prior to hiddbgExit. Only available with [7.0.0+].
Result hiddbgReleaseHdlsWorkBuffer(void);

/// Checks if the given HdlsHandle is still attached, where the result is written to isAttached.  Only available with [7.0.0+].
Result hiddbgIsHdlsVirtualDeviceAttached(u64 HdlsHandle, bool *isAttached);

/// Gets state for \ref HiddbgHdlsNpadAssignment. Only available with [7.0.0+].
Result hiddbgDumpHdlsNpadAssignmentState(HiddbgHdlsNpadAssignment *state);

/// Gets state for \ref HiddbgHdlsStateList. Only available with [7.0.0+].
Result hiddbgDumpHdlsStates(HiddbgHdlsStateList *state);

/// Sets state for \ref HiddbgHdlsNpadAssignment. Only available with [7.0.0+].
Result hiddbgApplyHdlsNpadAssignmentState(const HiddbgHdlsNpadAssignment *state, bool flag);

/// Sets state for \ref HiddbgHdlsStateList. Only available with [7.0.0+].
/// The HiddbgHdlsState will be applied for each HdlsHandle. If a HdlsHandle is not found, code similar to \ref hiddbgAttachHdlsVirtualDevice will run with the \ref HiddbgHdlsDeviceInfo, then it will continue with applying state with the new device.
Result hiddbgApplyHdlsStateList(const HiddbgHdlsStateList *state);

/// Attach a device with the input info, where the output handle is written to HdlsHandle. Only available with [7.0.0+].
Result hiddbgAttachHdlsVirtualDevice(u64 *HdlsHandle, const HiddbgHdlsDeviceInfo *info);

/// Detach the specified device. Only available with [7.0.0+].
Result hiddbgDetachHdlsVirtualDevice(u64 HdlsHandle);

/// Sets state for the specified device. Only available with [7.0.0+].
Result hiddbgSetHdlsState(u64 HdlsHandle, const HiddbgHdlsState *state);

