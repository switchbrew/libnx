/**
 * @file hiddbg.h
 * @brief hid:dbg service IPC wrapper.
 * @author yellows8
 */
#pragma once
#include "../types.h"
#include "../services/hid.h"
#include "../services/sm.h"

/// HdlsNpadAssignment
typedef struct {
    u8 unk_x0[0x208];         ///< Unknown
} HiddbgHdlsNpadAssignment;

/// HdlsStateList
typedef struct {
    u8 unk_x0[0x408];         ///< Unknown
} HiddbgHdlsStateList;

/// HdlsDeviceInfo
typedef struct {
    u32 type;                 ///< See \ref HidControllerType, only one bit can be set.
    u32 singleColorBody;      ///< RGBA Single Body Color
    u32 singleColorButtons;   ///< RGBA Single Buttons Color
    u8 unk_xc;                ///< Unknown
    u8 pad[0x3];
} HiddbgHdlsDeviceInfo;

/// HdlsState
typedef struct {
    u8 unk_x0[0x8];                                       ///< Unknown
    u32 unk_x8;                                           ///< Unknown, written to HidController +0x419C.
    u32 buttons;                                          ///< See \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];      ///< \ref JoystickPosition
    u32 unused;                                           ///< Unused
} HiddbgHdlsState;

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
Result hiddbgApplyHdlsStateList(const HiddbgHdlsStateList *state);

/// Attach a device with the input info, where the output handle is written to HdlsHandle. Only available with [7.0.0+].
Result hiddbgAttachHdlsVirtualDevice(u64 *HdlsHandle, const HiddbgHdlsDeviceInfo *info);

/// Detach the specified device. Only available with [7.0.0+].
Result hiddbgDetachHdlsVirtualDevice(u64 HdlsHandle);

/// Sets state for the specified device. Only available with [7.0.0+].
Result hiddbgSetHdlsState(u64 HdlsHandle, const HiddbgHdlsState *state);

