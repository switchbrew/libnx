/**
 * @file hiddbg.h
 * @brief hid:dbg service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../services/hid.h"
#include "../services/sm.h"

/// HdlsDeviceInfo
typedef struct {
    u32 type;                 ///< Only one bit can be set. BIT(0) = Pro-Controller, BIT(1) = Joy-Con Left, BIT(2) = Joy-Con Right, BIT(21) = unknown.
    u32 singleColorBody;      ///< RGBA Single Body Color
    u32 singleColorButtons;   ///< RGBA Single Buttons Color
    u8 type2;                 ///< Additional type field used with the above type field, if the value doesn't match one of the following a default is used. Type Pro-Controller: value 0x3 indicates that the controller is connected via USB. Type Joy-Con Left/Right: with value 0x2 the system doesn't list the controller in hid sharedmem. Type BIT(21): value 0x3 = unknown.
    u8 pad[0x3];              ///< Padding
} HiddbgHdlsDeviceInfo;

/// HdlsState
typedef struct {
    u8 powerConnected;                                    ///< powerConnected for the main PowerInfo, see \ref HidFlags.
    u8 flags;                                             ///< ORRed with powerConnected to set the value of the first byte for \ref HidFlags. For example, value 1 here will set isCharging for the main PowerInfo.
    u8 unk_x2[0x6];                                       ///< Unknown
    u32 batteryCharge;                                    ///< batteryCharge for the main PowerInfo, see \ref HidPowerInfo.
    u32 buttons;                                          ///< See \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];      ///< \ref JoystickPosition
    u8 unk_x20;                                           ///< Unused for input. Set with output from \ref hiddbgDumpHdlsStates.
    u8 padding[0x3];                                      ///< Padding
} HiddbgHdlsState;

/// HdlsNpadAssignment
typedef struct {
    u8 unk_x0[0x208];         ///< Unknown
} HiddbgHdlsNpadAssignment;

/// HdlsStateListEntry
typedef struct {
    u64 HdlsHandle;                             ///< HdlsHandle
    HiddbgHdlsDeviceInfo device;                ///< \ref HiddbgHdlsDeviceInfo. With \ref hiddbgApplyHdlsStateList this is only used when creating new devices.
    HiddbgHdlsState state;                      ///< \ref HiddbgHdlsState
} HiddbgHdlsStateListEntry;

/// HdlsStateList. This contains a list of all controllers, including non-virtual controllers.
typedef struct {
    s32 total_entries;                          ///< Total entries for the below entries.
    u32 pad;                                    ///< Padding
    HiddbgHdlsStateListEntry entries[0x10];     ///< \ref HiddbgHdlsStateListEntry
} HiddbgHdlsStateList;

Result hiddbgInitialize(void);
void hiddbgExit(void);

/// Initialize Hdls. Hdls is for virtual HID controllers. Only available with [7.0.0+].
Result hiddbgAttachHdlsWorkBuffer(void);

/// Exit Hdls, must be called at some point prior to hiddbgExit. Only available with [7.0.0+].
Result hiddbgReleaseHdlsWorkBuffer(void);

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

