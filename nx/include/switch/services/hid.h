/**
 * @file hid.h
 * @brief Human input device (hid) service IPC wrapper.
 * @author shinyquagsire23
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../kernel/event.h"
#include "../sf/service.h"

// Begin enums and output structs

/// HidMouseButton
typedef enum {
    HidMouseButton_Left    = BIT(0),
    HidMouseButton_Right   = BIT(1),
    HidMouseButton_Middle  = BIT(2),
    HidMouseButton_Forward = BIT(3),
    HidMouseButton_Back    = BIT(4),
} HidMouseButton;

/// HidKeyboardModifier
typedef enum {
    HidKeyboardModifier_Control       = BIT(0),
    HidKeyboardModifier_Shift         = BIT(1),
    HidKeyboardModifier_LeftAlt       = BIT(2),
    HidKeyboardModifier_RightAlt      = BIT(3),
    HidKeyboardModifier_Gui           = BIT(4),
    HidKeyboardModifier_CapsLock      = BIT(8),
    HidKeyboardModifier_ScrollLock    = BIT(9),
    HidKeyboardModifier_NumLock       = BIT(10),
    HidKeyboardModifier_Katakana      = BIT(11),
    HidKeyboardModifier_Hiragana      = BIT(12),
} HidKeyboardModifier;

/// HidKeyboardScancode
typedef enum {
    KBD_NONE = 0x00,
    KBD_ERR_OVF = 0x01,

    KBD_A = 0x04,
    KBD_B = 0x05,
    KBD_C = 0x06,
    KBD_D = 0x07,
    KBD_E = 0x08,
    KBD_F = 0x09,
    KBD_G = 0x0a,
    KBD_H = 0x0b,
    KBD_I = 0x0c,
    KBD_J = 0x0d,
    KBD_K = 0x0e,
    KBD_L = 0x0f,
    KBD_M = 0x10,
    KBD_N = 0x11,
    KBD_O = 0x12,
    KBD_P = 0x13,
    KBD_Q = 0x14,
    KBD_R = 0x15,
    KBD_S = 0x16,
    KBD_T = 0x17,
    KBD_U = 0x18,
    KBD_V = 0x19,
    KBD_W = 0x1a,
    KBD_X = 0x1b,
    KBD_Y = 0x1c,
    KBD_Z = 0x1d,

    KBD_1 = 0x1e,
    KBD_2 = 0x1f,
    KBD_3 = 0x20,
    KBD_4 = 0x21,
    KBD_5 = 0x22,
    KBD_6 = 0x23,
    KBD_7 = 0x24,
    KBD_8 = 0x25,
    KBD_9 = 0x26,
    KBD_0 = 0x27,

    KBD_ENTER = 0x28,
    KBD_ESC = 0x29,
    KBD_BACKSPACE = 0x2a,
    KBD_TAB = 0x2b,
    KBD_SPACE = 0x2c,
    KBD_MINUS = 0x2d,
    KBD_EQUAL = 0x2e,
    KBD_LEFTBRACE = 0x2f,
    KBD_RIGHTBRACE = 0x30,
    KBD_BACKSLASH = 0x31,
    KBD_HASHTILDE = 0x32,
    KBD_SEMICOLON = 0x33,
    KBD_APOSTROPHE = 0x34,
    KBD_GRAVE = 0x35,
    KBD_COMMA = 0x36,
    KBD_DOT = 0x37,
    KBD_SLASH = 0x38,
    KBD_CAPSLOCK = 0x39,

    KBD_F1 = 0x3a,
    KBD_F2 = 0x3b,
    KBD_F3 = 0x3c,
    KBD_F4 = 0x3d,
    KBD_F5 = 0x3e,
    KBD_F6 = 0x3f,
    KBD_F7 = 0x40,
    KBD_F8 = 0x41,
    KBD_F9 = 0x42,
    KBD_F10 = 0x43,
    KBD_F11 = 0x44,
    KBD_F12 = 0x45,

    KBD_SYSRQ = 0x46,
    KBD_SCROLLLOCK = 0x47,
    KBD_PAUSE = 0x48,
    KBD_INSERT = 0x49,
    KBD_HOME = 0x4a,
    KBD_PAGEUP = 0x4b,
    KBD_DELETE = 0x4c,
    KBD_END = 0x4d,
    KBD_PAGEDOWN = 0x4e,
    KBD_RIGHT = 0x4f,
    KBD_LEFT = 0x50,
    KBD_DOWN = 0x51,
    KBD_UP = 0x52,

    KBD_NUMLOCK = 0x53,
    KBD_KPSLASH = 0x54,
    KBD_KPASTERISK = 0x55,
    KBD_KPMINUS = 0x56,
    KBD_KPPLUS = 0x57,
    KBD_KPENTER = 0x58,
    KBD_KP1 = 0x59,
    KBD_KP2 = 0x5a,
    KBD_KP3 = 0x5b,
    KBD_KP4 = 0x5c,
    KBD_KP5 = 0x5d,
    KBD_KP6 = 0x5e,
    KBD_KP7 = 0x5f,
    KBD_KP8 = 0x60,
    KBD_KP9 = 0x61,
    KBD_KP0 = 0x62,
    KBD_KPDOT = 0x63,

    KBD_102ND = 0x64,
    KBD_COMPOSE = 0x65,
    KBD_POWER = 0x66,
    KBD_KPEQUAL = 0x67,

    KBD_F13 = 0x68,
    KBD_F14 = 0x69,
    KBD_F15 = 0x6a,
    KBD_F16 = 0x6b,
    KBD_F17 = 0x6c,
    KBD_F18 = 0x6d,
    KBD_F19 = 0x6e,
    KBD_F20 = 0x6f,
    KBD_F21 = 0x70,
    KBD_F22 = 0x71,
    KBD_F23 = 0x72,
    KBD_F24 = 0x73,

    KBD_OPEN = 0x74,
    KBD_HELP = 0x75,
    KBD_PROPS = 0x76,
    KBD_FRONT = 0x77,
    KBD_STOP = 0x78,
    KBD_AGAIN = 0x79,
    KBD_UNDO = 0x7a,
    KBD_CUT = 0x7b,
    KBD_COPY = 0x7c,
    KBD_PASTE = 0x7d,
    KBD_FIND = 0x7e,
    KBD_MUTE = 0x7f,
    KBD_VOLUMEUP = 0x80,
    KBD_VOLUMEDOWN = 0x81,
    KBD_CAPSLOCK_ACTIVE = 0x82 ,
    KBD_NUMLOCK_ACTIVE = 0x83 ,
    KBD_SCROLLLOCK_ACTIVE = 0x84 ,
    KBD_KPCOMMA = 0x85,

    KBD_KPLEFTPAREN = 0xb6,
    KBD_KPRIGHTPAREN = 0xb7,

    KBD_LEFTCTRL = 0xe0,
    KBD_LEFTSHIFT = 0xe1,
    KBD_LEFTALT = 0xe2,
    KBD_LEFTMETA = 0xe3,
    KBD_RIGHTCTRL = 0xe4,
    KBD_RIGHTSHIFT = 0xe5,
    KBD_RIGHTALT = 0xe6,
    KBD_RIGHTMETA = 0xe7,

    KBD_MEDIA_PLAYPAUSE = 0xe8,
    KBD_MEDIA_STOPCD = 0xe9,
    KBD_MEDIA_PREVIOUSSONG = 0xea,
    KBD_MEDIA_NEXTSONG = 0xeb,
    KBD_MEDIA_EJECTCD = 0xec,
    KBD_MEDIA_VOLUMEUP = 0xed,
    KBD_MEDIA_VOLUMEDOWN = 0xee,
    KBD_MEDIA_MUTE = 0xef,
    KBD_MEDIA_WWW = 0xf0,
    KBD_MEDIA_BACK = 0xf1,
    KBD_MEDIA_FORWARD = 0xf2,
    KBD_MEDIA_STOP = 0xf3,
    KBD_MEDIA_FIND = 0xf4,
    KBD_MEDIA_SCROLLUP = 0xf5,
    KBD_MEDIA_SCROLLDOWN = 0xf6,
    KBD_MEDIA_EDIT = 0xf7,
    KBD_MEDIA_SLEEP = 0xf8,
    KBD_MEDIA_COFFEE = 0xf9,
    KBD_MEDIA_REFRESH = 0xfa,
    KBD_MEDIA_CALC = 0xfb
} HidKeyboardScancode;

/// HID controller IDs
typedef enum {
    HidNpadIdType_No1      = 0,    ///< Player 1 controller
    HidNpadIdType_No2      = 1,    ///< Player 2 controller
    HidNpadIdType_No3      = 2,    ///< Player 3 controller
    HidNpadIdType_No4      = 3,    ///< Player 4 controller
    HidNpadIdType_No5      = 4,    ///< Player 5 controller
    HidNpadIdType_No6      = 5,    ///< Player 6 controller
    HidNpadIdType_No7      = 6,    ///< Player 7 controller
    HidNpadIdType_No8      = 7,    ///< Player 8 controller
    HidNpadIdType_Other    = 0x10, ///< Other controller
    HidNpadIdType_Handheld = 0x20, ///< Handheld mode controls
} HidNpadIdType;

/// HID controller styles
typedef enum {
    HidNpadStyleTag_NpadFullKey       = BIT(0),       ///< Pro Controller
    HidNpadStyleTag_NpadHandheld      = BIT(1),       ///< Joy-Con controller in handheld mode
    HidNpadStyleTag_NpadJoyDual       = BIT(2),       ///< Joy-Con controller in dual mode
    HidNpadStyleTag_NpadJoyLeft       = BIT(3),       ///< Joy-Con left controller in single mode
    HidNpadStyleTag_NpadJoyRight      = BIT(4),       ///< Joy-Con right controller in single mode
    HidNpadStyleTag_NpadGc            = BIT(5),       ///< GameCube controller
    HidNpadStyleTag_NpadPalma         = BIT(6),       ///< Poké Ball Plus controller
    HidNpadStyleTag_NpadLark          = BIT(7),       ///< NES/Famicom controller
    HidNpadStyleTag_NpadHandheldLark  = BIT(8),       ///< NES/Famicom controller in handheld mode
    HidNpadStyleTag_NpadLucia         = BIT(9),       ///< SNES controller
    HidNpadStyleTag_Npad10            = BIT(10),
    HidNpadStyleTag_NpadSystemExt     = BIT(29),      ///< Generic external controller
    HidNpadStyleTag_NpadSystem        = BIT(30),      ///< Generic controller
} HidNpadStyleTag;

/// HidColorAttribute
typedef enum {
    HidColorAttribute_Ok              = 0,            ///< Ok
    HidColorAttribute_ReadError       = 1,            ///< ReadError
    HidColorAttribute_NoController    = 2,            ///< NoController
} HidColorAttribute;

/// HidControllerKeys
typedef enum {
    KEY_A            = BIT(0),       ///< A
    KEY_B            = BIT(1),       ///< B
    KEY_X            = BIT(2),       ///< X
    KEY_Y            = BIT(3),       ///< Y
    KEY_LSTICK       = BIT(4),       ///< Left Stick Button
    KEY_RSTICK       = BIT(5),       ///< Right Stick Button
    KEY_L            = BIT(6),       ///< L
    KEY_R            = BIT(7),       ///< R
    KEY_ZL           = BIT(8),       ///< ZL
    KEY_ZR           = BIT(9),       ///< ZR
    KEY_PLUS         = BIT(10),      ///< Plus
    KEY_MINUS        = BIT(11),      ///< Minus
    KEY_DLEFT        = BIT(12),      ///< D-Pad Left
    KEY_DUP          = BIT(13),      ///< D-Pad Up
    KEY_DRIGHT       = BIT(14),      ///< D-Pad Right
    KEY_DDOWN        = BIT(15),      ///< D-Pad Down
    KEY_LSTICK_LEFT  = BIT(16),      ///< Left Stick Left
    KEY_LSTICK_UP    = BIT(17),      ///< Left Stick Up
    KEY_LSTICK_RIGHT = BIT(18),      ///< Left Stick Right
    KEY_LSTICK_DOWN  = BIT(19),      ///< Left Stick Down
    KEY_RSTICK_LEFT  = BIT(20),      ///< Right Stick Left
    KEY_RSTICK_UP    = BIT(21),      ///< Right Stick Up
    KEY_RSTICK_RIGHT = BIT(22),      ///< Right Stick Right
    KEY_RSTICK_DOWN  = BIT(23),      ///< Right Stick Down
    KEY_SL_LEFT      = BIT(24),      ///< SL on Left Joy-Con
    KEY_SR_LEFT      = BIT(25),      ///< SR on Left Joy-Con
    KEY_SL_RIGHT     = BIT(26),      ///< SL on Right Joy-Con
    KEY_SR_RIGHT     = BIT(27),      ///< SR on Right Joy-Con

    KEY_HOME         = BIT(18),      ///< HOME button, only available for use with HiddbgHdlsState::buttons.
    KEY_CAPTURE      = BIT(19),      ///< Capture button, only available for use with HiddbgHdlsState::buttons.

    // Pseudo-key for at least one finger on the touch screen
    KEY_TOUCH       = BIT(28),

    KEY_NES_HANDHELD_LEFT_B = BIT(30),   ///< Left B button on NES controllers in Handheld mode.

    // Buttons by orientation (for single Joy-Con), also works with Joy-Con pairs, Pro Controller
    KEY_JOYCON_RIGHT = BIT(0),
    KEY_JOYCON_DOWN  = BIT(1),
    KEY_JOYCON_UP    = BIT(2),
    KEY_JOYCON_LEFT  = BIT(3),

    // Generic catch-all directions, also works for single Joy-Con
    KEY_UP    = KEY_DUP     | KEY_LSTICK_UP    | KEY_RSTICK_UP,    ///< D-Pad Up or Sticks Up
    KEY_DOWN  = KEY_DDOWN   | KEY_LSTICK_DOWN  | KEY_RSTICK_DOWN,  ///< D-Pad Down or Sticks Down
    KEY_LEFT  = KEY_DLEFT   | KEY_LSTICK_LEFT  | KEY_RSTICK_LEFT,  ///< D-Pad Left or Sticks Left
    KEY_RIGHT = KEY_DRIGHT  | KEY_LSTICK_RIGHT | KEY_RSTICK_RIGHT, ///< D-Pad Right or Sticks Right
    KEY_SL    = KEY_SL_LEFT | KEY_SL_RIGHT,                        ///< SL on Left or Right Joy-Con
    KEY_SR    = KEY_SR_LEFT | KEY_SR_RIGHT,                        ///< SR on Left or Right Joy-Con
} HidControllerKeys;

/// HidControllerJoystick
typedef enum {
    JOYSTICK_LEFT  = 0,
    JOYSTICK_RIGHT = 1,

    JOYSTICK_NUM_STICKS = 2,
} HidControllerJoystick;

/// HidTouchAttribute
typedef enum {
    HidTouchAttribute_Start            = BIT(0),    ///< Start
    HidTouchAttribute_End              = BIT(1),    ///< End
} HidTouchAttribute;

/// HidMouseAttribute
typedef enum {
    HidMouseAttribute_Transferable     = BIT(0),    ///< Transferable
    HidMouseAttribute_IsConnected      = BIT(1),    ///< IsConnected
} HidMouseAttribute;

/// HidNpadAttribute
typedef enum {
    HidNpadAttribute_IsConnected          = BIT(0),    ///< IsConnected
    HidNpadAttribute_IsWired              = BIT(1),    ///< IsWired
    HidNpadAttribute_IsLeftConnected      = BIT(2),    ///< IsLeftConnected
    HidNpadAttribute_IsLeftWired          = BIT(3),    ///< IsLeftWired
    HidNpadAttribute_IsRightConnected     = BIT(4),    ///< IsRightConnected
    HidNpadAttribute_IsRightWired         = BIT(5),    ///< IsRightWired
} HidNpadAttribute;

/// HidSixAxisSensorAttribute
typedef enum {
    HidSixAxisSensorAttribute_IsConnected     = BIT(0),    ///< IsConnected
    HidSixAxisSensorAttribute_IsInterpolated  = BIT(1),    ///< IsInterpolated
} HidSixAxisSensorAttribute;

/// HidGestureAttribute
typedef enum {
    HidGestureAttribute_IsNewTouch            = BIT(0),    ///< IsNewTouch
    HidGestureAttribute_IsDoubleTap           = BIT(1),    ///< IsDoubleTap
} HidGestureAttribute;

/// HidGestureDirection
typedef enum {
    HidGestureDirection_None           = 0,    ///< None
    HidGestureDirection_Left           = 1,    ///< Left
    HidGestureDirection_Up             = 2,    ///< Up
    HidGestureDirection_Right          = 3,    ///< Right
    HidGestureDirection_Down           = 4,    ///< Down
} HidGestureDirection;

/// HidGestureType
typedef enum {
    HidGestureType_Idle                = 0,    ///< Idle
    HidGestureType_Complete            = 1,    ///< Complete
    HidGestureType_Cancel              = 2,    ///< Cancel
    HidGestureType_Touch               = 3,    ///< Touch
    HidGestureType_Press               = 4,    ///< Press
    HidGestureType_Tap                 = 5,    ///< Tap
    HidGestureType_Pan                 = 6,    ///< Pan
    HidGestureType_Swipe               = 7,    ///< Swipe
    HidGestureType_Pinch               = 8,    ///< Pinch
    HidGestureType_Rotate              = 9,    ///< Rotate
} HidGestureType;

/// HidControllerID
typedef enum {
    CONTROLLER_PLAYER_1 = 0,
    CONTROLLER_PLAYER_2 = 1,
    CONTROLLER_PLAYER_3 = 2,
    CONTROLLER_PLAYER_4 = 3,
    CONTROLLER_PLAYER_5 = 4,
    CONTROLLER_PLAYER_6 = 5,
    CONTROLLER_PLAYER_7 = 6,
    CONTROLLER_PLAYER_8 = 7,
    CONTROLLER_HANDHELD = 8,
    CONTROLLER_UNKNOWN  = 9,
    CONTROLLER_P1_AUTO = 10, ///< Not an actual HID-sysmodule ID. Only for hidKeys*()/hidJoystickRead()/hidSixAxisSensorValuesRead()/hidIsControllerConnected(). Automatically uses CONTROLLER_PLAYER_1 when connected, otherwise uses CONTROLLER_HANDHELD.
} HidControllerID;

/// GyroscopeZeroDriftMode
typedef enum {
    HidGyroscopeZeroDriftMode_Loose    = 0,   ///< Loose
    HidGyroscopeZeroDriftMode_Standard = 1,   ///< Standard
    HidGyroscopeZeroDriftMode_Tight    = 2,   ///< Tight
} HidGyroscopeZeroDriftMode;

/// JoyHoldType
typedef enum {
    HidJoyHoldType_Default    = 0, ///< Default / Joy-Con held vertically.
    HidJoyHoldType_Horizontal = 1, ///< Joy-Con held horizontally with HID state orientation adjustment.
} HidJoyHoldType;

/// NpadJoyAssignmentMode
typedef enum {
    HidNpadJoyAssignmentMode_Dual   = 0,       ///< Dual (Set by \ref hidSetNpadJoyAssignmentModeDual)
    HidNpadJoyAssignmentMode_Single = 1,       ///< Single (Set by hidSetNpadJoyAssignmentModeSingle*())
} HidNpadJoyAssignmentMode;

/// DeviceType
typedef enum {
    HidDeviceTypeBits_FullKey       = BIT(0),  ///< Pro Controller and Gc controller.
    HidDeviceTypeBits_Unknown1      = BIT(1),  ///< Unknown.
    HidDeviceTypeBits_HandheldLeft  = BIT(2),  ///< Joy-Con/Famicom/NES left controller in handheld mode.
    HidDeviceTypeBits_HandheldRight = BIT(3),  ///< Joy-Con/Famicom/NES right controller in handheld mode.
    HidDeviceTypeBits_JoyLeft       = BIT(4),  ///< Joy-Con left controller.
    HidDeviceTypeBits_JoyRight      = BIT(5),  ///< Joy-Con right controller.
    HidDeviceTypeBits_Palma         = BIT(6),  ///< Poké Ball Plus controller.
    HidDeviceTypeBits_LarkLeftHVC   = BIT(7),  ///< Famicom left controller.
    HidDeviceTypeBits_LarkRightHVC  = BIT(8),  ///< Famicom right controller (with microphone).
    HidDeviceTypeBits_LarkLeftNES   = BIT(9),  ///< NES left controller.
    HidDeviceTypeBits_LarkRightNES  = BIT(10), ///< NES right controller.
    HidDeviceTypeBits_SystemExt     = BIT(15), ///< Generic external controller.
    HidDeviceTypeBits_System        = BIT(31), ///< Generic controller.
} HidDeviceTypeBits;

/// Internal DeviceType for [9.0.0+]. Converted to/from the pre-9.0.0 version of this by the hiddbg funcs.
typedef enum {
    HidDeviceType_JoyRight1       = 1,   ///< ::HidDeviceTypeBits_JoyRight
    HidDeviceType_JoyLeft2        = 2,   ///< ::HidDeviceTypeBits_JoyLeft
    HidDeviceType_FullKey3        = 3,   ///< ::HidDeviceTypeBits_FullKey
    HidDeviceType_JoyLeft4        = 4,   ///< ::HidDeviceTypeBits_JoyLeft
    HidDeviceType_JoyRight5       = 5,   ///< ::HidDeviceTypeBits_JoyRight
    HidDeviceType_FullKey6        = 6,   ///< ::HidDeviceTypeBits_FullKey
    HidDeviceType_LarkLeftHVC     = 7,   ///< ::HidDeviceTypeBits_LarkLeftHVC
    HidDeviceType_LarkRightHVC    = 8,   ///< ::HidDeviceTypeBits_LarkRightHVC
    HidDeviceType_LarkLeftNES     = 9,   ///< ::HidDeviceTypeBits_LarkLeftNES
    HidDeviceType_LarkRightNES    = 10,  ///< ::HidDeviceTypeBits_LarkRightNES
    HidDeviceType_Palma           = 12,  ///< [9.0.0+] ::HidDeviceTypeBits_Palma
    HidDeviceType_FullKey13       = 13,  ///< ::HidDeviceTypeBits_FullKey
    HidDeviceType_FullKey15       = 15,  ///< ::HidDeviceTypeBits_FullKey
    HidDeviceType_System19        = 19,  ///< ::HidDeviceTypeBits_System with \ref HidNpadStyleTag |= ::HidNpadStyleTag_NpadFullKey.
    HidDeviceType_System20        = 20,  ///< ::HidDeviceTypeBits_System with \ref HidNpadStyleTag |= ::HidNpadStyleTag_NpadJoyDual.
    HidDeviceType_System21        = 21,  ///< ::HidDeviceTypeBits_System with \ref HidNpadStyleTag |= ::HidNpadStyleTag_NpadJoyDual.
} HidDeviceType;

/// AppletFooterUiType
typedef enum {
    HidAppletFooterUiType_None                             = 0,     ///< None
    HidAppletFooterUiType_HandheldNone                     = 1,     ///< HandheldNone
    HidAppletFooterUiType_HandheldJoyConLeftOnly           = 2,     ///< HandheldJoyConLeftOnly
    HidAppletFooterUiType_HandheldJoyConRightOnly          = 3,     ///< HandheldJoyConRightOnly
    HidAppletFooterUiType_HandheldJoyConLeftJoyConRight    = 4,     ///< HandheldJoyConLeftJoyConRight
    HidAppletFooterUiType_JoyDual                          = 5,     ///< JoyDual
    HidAppletFooterUiType_JoyDualLeftOnly                  = 6,     ///< JoyDualLeftOnly
    HidAppletFooterUiType_JoyDualRightOnly                 = 7,     ///< JoyDualRightOnly
    HidAppletFooterUiType_JoyLeftHorizontal                = 8,     ///< JoyLeftHorizontal
    HidAppletFooterUiType_JoyLeftVertical                  = 9,     ///< JoyLeftVertical
    HidAppletFooterUiType_JoyRightHorizontal               = 10,    ///< JoyRightHorizontal
    HidAppletFooterUiType_JoyRightVertical                 = 11,    ///< JoyRightVertical
    HidAppletFooterUiType_SwitchProController              = 12,    ///< SwitchProController
    HidAppletFooterUiType_CompatibleProController          = 13,    ///< CompatibleProController
    HidAppletFooterUiType_CompatibleJoyCon                 = 14,    ///< CompatibleJoyCon
    HidAppletFooterUiType_LarkHvc1                         = 15,    ///< LarkHvc1
    HidAppletFooterUiType_LarkHvc2                         = 16,    ///< LarkHvc2
    HidAppletFooterUiType_LarkNesLeft                      = 17,    ///< LarkNesLeft
    HidAppletFooterUiType_LarkNesRight                     = 18,    ///< LarkNesRight
    HidAppletFooterUiType_Lucia                            = 19,    ///< Lucia
    HidAppletFooterUiType_Verification                     = 20,    ///< Verification
} HidAppletFooterUiType;

/// NpadInterfaceType
typedef enum {
    HidNpadInterfaceType_Bluetooth = 1,    ///< Bluetooth.
    HidNpadInterfaceType_Rail      = 2,    ///< Rail.
    HidNpadInterfaceType_USB       = 3,    ///< USB.
    HidNpadInterfaceType_Unknown4  = 4,    ///< Unknown.
} HidNpadInterfaceType;

/// XcdInterfaceType
typedef enum {
    XcdInterfaceType_Bluetooth  = BIT(0),
    XcdInterfaceType_Uart       = BIT(1),
    XcdInterfaceType_Usb        = BIT(2),
    XcdInterfaceType_FieldSet   = BIT(7),
} XcdInterfaceType;

/// NpadLarkType
typedef enum {
    HidNpadLarkType_Invalid     = 0,    ///< Invalid
    HidNpadLarkType_H1          = 1,    ///< H1
    HidNpadLarkType_H2          = 2,    ///< H2
    HidNpadLarkType_NL          = 3,    ///< NL
    HidNpadLarkType_NR          = 4,    ///< NR
} HidNpadLarkType;

/// NpadLuciaType
typedef enum {
    HidNpadLuciaType_Invalid    = 0,    ///< Invalid
    HidNpadLuciaType_J          = 1,    ///< J
    HidNpadLuciaType_E          = 2,    ///< E
    HidNpadLuciaType_U          = 3,    ///< U
} HidNpadLuciaType;

/// touchPosition
typedef struct touchPosition {
    u32 id;
    u32 px;
    u32 py;
    u32 dx;
    u32 dy;
    u32 angle;
} touchPosition;

/// JoystickPosition
typedef struct JoystickPosition {
    s32 dx;
    s32 dy;
} JoystickPosition;

/// MousePosition
typedef struct MousePosition {
    s32 x;
    s32 y;
    s32 velocityX;
    s32 velocityY;
    s32 scrollVelocityX;
    s32 scrollVelocityY;
} MousePosition;

/// HidVector
typedef struct HidVector {
    float x;
    float y;
    float z;
} HidVector;

/// SixAxisSensorValues
typedef struct SixAxisSensorValues {
    HidVector accelerometer;
    HidVector gyroscope;
    HidVector unk;
    HidVector orientation[3];
} SixAxisSensorValues;

#define JOYSTICK_MAX (0x7FFF)
#define JOYSTICK_MIN (-0x7FFF)

// End enums and output structs

/// HidCommonLifoHeader
typedef struct HidCommonLifoHeader {
    u64 sampling_number;                        ///< SamplingNumber
    u64 buffer_count;                           ///< BufferCount
    u64 tail;                                   ///< Tail
    u64 count;                                  ///< Count
} HidCommonLifoHeader;

// Begin HidTouchScreen

/// HidTouchState
typedef struct HidTouchState {
    u64 delta_time;                             ///< DeltaTime
    u32 attributes;                             ///< Bitfield of \ref HidTouchAttribute.
    u32 finger_id;                              ///< FingerId
    u32 x;                                      ///< X
    u32 y;                                      ///< Y
    u32 diameter_x;                             ///< DiameterX
    u32 diameter_y;                             ///< DiameterY
    u32 rotation_angle;                         ///< RotationAngle
    u32 reserved;                               ///< Reserved
} HidTouchState;

/// HidTouchScreenState
typedef struct HidTouchScreenState {
    u64 sampling_number;                        ///< SamplingNumber
    s32 count;                                  ///< Number of entries in the touches array.
    u32 reserved;                               ///< Reserved
    HidTouchState touches[16];                  ///< Array of \ref HidTouchState, with the above count.
} HidTouchScreenState;

/// HidTouchScreenStateAtomicStorage
typedef struct HidTouchScreenStateAtomicStorage {
    u64 sampling_number;                             ///< SamplingNumber
    HidTouchScreenState state;                       ///< \ref HidTouchScreenState
} HidTouchScreenStateAtomicStorage;

/// HidTouchScreenLifo
typedef struct HidTouchScreenLifo {
    HidCommonLifoHeader header;                      ///< \ref HidCommonLifoHeader
    HidTouchScreenStateAtomicStorage storage[17];    ///< \ref HidTouchScreenStateAtomicStorage
} HidTouchScreenLifo;

/// HidTouchScreenSharedMemoryFormat
typedef struct HidTouchScreenSharedMemoryFormat {
    HidTouchScreenLifo lifo;
    u8 padding[0x3c8];
} HidTouchScreenSharedMemoryFormat;

// End HidTouchScreen

// Begin HidMouse

/// HidMouseState
typedef struct HidMouseState {
    u64 sampling_number;                        ///< SamplingNumber
    MousePosition position;                     ///< \ref MousePosition
    u32 buttons;                                ///< Bitfield of \ref HidMouseButton.
    u32 attributes;                             ///< Bitfield of \ref HidMouseAttribute.
} HidMouseState;

/// HidMouseStateAtomicStorage
typedef struct HidMouseStateAtomicStorage {
    u64 sampling_number;                        ///< SamplingNumber
    HidMouseState state;
} HidMouseStateAtomicStorage;

/// HidMouseLifo
typedef struct HidMouseLifo {
    HidCommonLifoHeader header;
    HidMouseStateAtomicStorage storage[17];
} HidMouseLifo;

/// HidMouseSharedMemoryFormat
typedef struct HidMouseSharedMemoryFormat {
    HidMouseLifo lifo;
    u8 padding[0xB0];
} HidMouseSharedMemoryFormat;

// End HidMouse

// Begin HidKeyboard

/// HidKeyboardState
typedef struct HidKeyboardState {
    u64 sampling_number;                        ///< SamplingNumber
    u64 modifiers;                              ///< Bitfield of \ref HidKeyboardModifier.
    u64 keys[4];
} HidKeyboardState;

/// HidKeyboardStateAtomicStorage
typedef struct HidKeyboardStateAtomicStorage {
    u64 sampling_number;                        ///< SamplingNumber
    HidKeyboardState state;
} HidKeyboardStateAtomicStorage;

/// HidKeyboardLifo
typedef struct HidKeyboardLifo {
    HidCommonLifoHeader header;
    HidKeyboardStateAtomicStorage storage[17];
} HidKeyboardLifo;

/// HidKeyboardSharedMemoryFormat
typedef struct HidKeyboardSharedMemoryFormat {
    HidKeyboardLifo lifo;
    u8 padding[0x28];
} HidKeyboardSharedMemoryFormat;

// End HidKeyboard

// Begin HidNpad

/// Npad colors.
/// Color fields are zero when not set.
typedef struct HidNpadControllerColor {
    u32 main;    ///< RGBA Body Color
    u32 sub;     ///< RGBA Buttons Color
} HidNpadControllerColor;

/// HidNpadFullKeyColorState
typedef struct HidNpadFullKeyColorState {
    u32 attribute;                               ///< \ref HidColorAttribute
    HidNpadControllerColor full_key;             ///< \ref HidNpadControllerColor FullKey
} HidNpadFullKeyColorState;

/// HidNpadJoyColorState
typedef struct HidNpadJoyColorState {
    u32 attribute;                               ///< \ref HidColorAttribute
    HidNpadControllerColor left;                 ///< \ref HidNpadControllerColor Left
    HidNpadControllerColor right;                ///< \ref HidNpadControllerColor Right
} HidNpadJoyColorState;

/// HidNpadCommonState
typedef struct HidNpadCommonState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    u32 reserved;
} HidNpadCommonState;

typedef HidNpadCommonState HidNpadFullKeyState;
typedef HidNpadCommonState HidNpadHandheldState;
typedef HidNpadCommonState HidNpadJoyDualState;
typedef HidNpadCommonState HidNpadJoyLeftState;
typedef HidNpadCommonState HidNpadJoyRightState;

/// HidNpadGcState
typedef struct HidNpadGcState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    u32 trigger_l;                                      ///< L analog trigger. Valid range: 0x0-0x7FFF.
    u32 trigger_r;                                      ///< R analog trigger. Valid range: 0x0-0x7FFF.
    u32 pad;
} HidNpadGcState;

typedef HidNpadCommonState HidNpadPalmaState;

/// HidNpadLarkState
typedef struct HidNpadLarkState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];    ///< Joysticks state are always zero.
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLarkType lark_type_l_and_main;               ///< \ref HidNpadLarkType LarkTypeLAndMain
} HidNpadLarkState;

/// HidNpadHandheldLarkState
typedef struct HidNpadHandheldLarkState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLarkType lark_type_l_and_main;               ///< \ref HidNpadLarkType LarkTypeLAndMain
    HidNpadLarkType lark_type_r;                        ///< \ref HidNpadLarkType LarkTypeR
    u32 pad;
} HidNpadHandheldLarkState;

/// HidNpadLuciaState
typedef struct HidNpadLuciaState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidControllerKeys.
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];    ///< Joysticks state are always zero.
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLuciaType lucia_type;                        ///< \ref HidNpadLuciaType
} HidNpadLuciaState;

typedef HidNpadCommonState HidNpadSystemExtState;
typedef HidNpadCommonState HidNpadSystemState; ///< Joysticks state are always zero. Only the following button bits are available: KEY_A, KEY_B, KEY_X, KEY_Y, KEY_DLEFT, KEY_DUP, KEY_DRIGHT, KEY_DDOWN, KEY_L, KEY_R.

/// HidNpadCommonStateAtomicStorage
typedef struct HidNpadCommonStateAtomicStorage {
    u64 sampling_number;                                ///< SamplingNumber
    HidNpadCommonState state;
} HidNpadCommonStateAtomicStorage;

/// HidNpadCommonLifo
typedef struct HidNpadCommonLifo {
    HidCommonLifoHeader header;
    HidNpadCommonStateAtomicStorage storage[17];
} HidNpadCommonLifo;

/// HidNpadGcTriggerState
typedef struct HidNpadGcTriggerState {
    u64 sampling_number;                                ///< SamplingNumber
    u32 trigger_l;
    u32 trigger_r;
} HidNpadGcTriggerState;

/// HidNpadGcTriggerStateAtomicStorage
typedef struct HidNpadGcTriggerStateAtomicStorage {
    u64 sampling_number;                                ///< SamplingNumber
    HidNpadGcTriggerState state;
} HidNpadGcTriggerStateAtomicStorage;

/// HidNpadGcTriggerLifo
typedef struct HidNpadGcTriggerLifo {
    HidCommonLifoHeader header;
    HidNpadGcTriggerStateAtomicStorage storage[17];
} HidNpadGcTriggerLifo;

/// HidSixAxisSensorState
typedef struct HidSixAxisSensorState {
    u64 delta_time;                                     ///< DeltaTime
    u64 sampling_number;                                ///< SamplingNumber
    SixAxisSensorValues values;
    u32 attributes;                                     ///< Bitfield of \ref HidSixAxisSensorAttribute.
    u32 reserved;                                       ///< Reserved
} HidSixAxisSensorState;

/// HidSixAxisSensorStateAtomicStorage
typedef struct HidSixAxisSensorStateAtomicStorage {
    u64 sampling_number;                                ///< SamplingNumber
    HidSixAxisSensorState state;
} HidSixAxisSensorStateAtomicStorage;

/// HidNpadSixAxisSensorLifo
typedef struct HidNpadSixAxisSensorLifo {
    HidCommonLifoHeader header;
    HidSixAxisSensorStateAtomicStorage storage[17];
} HidNpadSixAxisSensorLifo;

/// NpadSystemProperties
typedef struct {
    u64 powerInfo : 6;                                               ///< Use \ref hidGetNpadPowerInfo instead of accessing this directly.

    u64 bit6 : 1;                                                    ///< Unused
    u64 bit7 : 1;                                                    ///< Unused
    u64 bit8 : 1;                                                    ///< Unused
    u64 is_unsupported_button_pressed_on_npad_system : 1;            ///< IsUnsupportedButtonPressedOnNpadSystem
    u64 is_unsupported_button_pressed_on_npad_system_ext : 1;        ///< IsUnsupportedButtonPressedOnNpadSystemExt

    u64 is_abxy_button_oriented : 1;                                 ///< IsAbxyButtonOriented
    u64 is_sl_sr_button_oriented : 1;                                ///< IsSlSrButtonOriented
    u64 is_plus_available : 1;                                       ///< [4.0.0+] IsPlusAvailable
    u64 is_minus_available : 1;                                      ///< [4.0.0+] IsMinusAvailable
    u64 is_directional_buttons_available : 1;                        ///< [8.0.0+] IsDirectionalButtonsAvailable

    u64 unused : 48;                                                 ///< Unused
} HidNpadSystemProperties;

/// NpadSystemButtonProperties
typedef struct {
    u32 is_unintended_home_button_input_protection_enabled : 1;      ///< IsUnintendedHomeButtonInputProtectionEnabled
} HidNpadSystemButtonProperties;

/// HidPowerInfo
typedef struct {
    bool is_powered;      ///< IsPowered
    bool is_charging;     ///< IsCharging
    u8 reserved[6];       ///< Reserved
    u32 battery_level;    ///< BatteryLevel, always 0-4.
} HidPowerInfo;

/// XcdDeviceHandle
typedef struct XcdDeviceHandle {
    u64 handle;
} XcdDeviceHandle;

/// HidNfcXcdDeviceHandleStateImpl
typedef struct HidNfcXcdDeviceHandleStateImpl {
    XcdDeviceHandle handle;
    u8 is_available;
    u8 is_activated;
    u8 reserved[6];
    u64 sampling_number;                                ///< SamplingNumber
} HidNfcXcdDeviceHandleStateImpl;

/// HidNfcXcdDeviceHandleStateImplAtomicStorage
typedef struct HidNfcXcdDeviceHandleStateImplAtomicStorage {
    u64 sampling_number;                                ///< SamplingNumber
    HidNfcXcdDeviceHandleStateImpl state;               ///< \ref HidNfcXcdDeviceHandleStateImpl
} HidNfcXcdDeviceHandleStateImplAtomicStorage;

/// HidNfcXcdDeviceHandleState
typedef struct HidNfcXcdDeviceHandleState {
    HidCommonLifoHeader header;
    HidNfcXcdDeviceHandleStateImplAtomicStorage storage[2];
} HidNfcXcdDeviceHandleState;

/// HidNpadInternalState
typedef struct HidNpadInternalState {
    u32 style_set;                               ///< Bitfield of \ref HidNpadStyleTag.
    u32 joy_assignment_mode;                     ///< \ref HidNpadJoyAssignmentMode
    HidNpadFullKeyColorState full_key_color;     ///< \ref HidNpadFullKeyColorState
    HidNpadJoyColorState joy_color;              ///< \ref HidNpadJoyColorState
    HidNpadCommonLifo layouts[7];
    HidNpadSixAxisSensorLifo sixaxis[6];
    u32 device_type;                             ///< Bitfield of \ref HidDeviceTypeBits.
    u32 reserved;                                ///< Reserved
    HidNpadSystemProperties system_properties;
    HidNpadSystemButtonProperties system_button_properties;
    u32 battery_level[3];
    union {
        struct { // [1.0.0-3.0.2]
            HidNfcXcdDeviceHandleState nfc_xcd_device_handle;
        };

        struct {
            u32 applet_footer_ui_attribute;                                         ///< Bitfield of AppletFooterUiAttribute.
            u8 applet_footer_ui_type;                                               ///< \ref HidAppletFooterUiType
            u8 reserved_x41AD[0x5B];
        };
    };
    u8 reserved_x4208[0x20];                                                        ///< Mutex on pre-10.0.0.
    HidNpadGcTriggerLifo gc_trigger_lifo;
    u32 lark_type_l_and_main;                                                       ///< \ref HidNpadLarkType
    u32 lark_type_r;                                                                ///< \ref HidNpadLarkType
    u32 lucia_type;                                                                 ///< \ref HidNpadLuciaType
    u32 unk_x43EC;
} HidNpadInternalState;

/// HidNpadSharedMemoryEntry
typedef struct HidNpadSharedMemoryEntry {
    HidNpadInternalState internal_state;
    u8 pad[0xC10];
} HidNpadSharedMemoryEntry;

/// HidNpadSharedMemoryFormat
typedef struct HidNpadSharedMemoryFormat {
    HidNpadSharedMemoryEntry entries[10];
} HidNpadSharedMemoryFormat;

// End HidNpad

// Begin HidGesture

/// HidGesturePoint
typedef struct HidGesturePoint {
    u32 x;                                              ///< X
    u32 y;                                              ///< Y
} HidGesturePoint;

/// HidGestureState
typedef struct HidGestureState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 context_number;                                 ///< ContextNumber
    u32 type;                                           ///< \ref HidGestureType
    u32 direction;                                      ///< \ref HidGestureDirection
    u32 x;                                              ///< X
    u32 y;                                              ///< Y
    s32 delta_x;                                        ///< DeltaX
    s32 delta_y;                                        ///< DeltaY
    float velocity_x;                                   ///< VelocityX
    float velocity_y;                                   ///< VelocityY
    u32 attributes;                                     ///< Bitfield of \ref HidGestureAttribute.
    u32 scale;                                          ///< Scale
    u32 rotation_angle;                                 ///< RotationAngle
    s32 point_count;                                    ///< Number of entries in the points array.
    HidGesturePoint points[4];                          ///< Array of \ref HidGesturePoint with the above count.
} HidGestureState;

/// HidGestureDummyStateAtomicStorage
typedef struct HidGestureDummyStateAtomicStorage {
    u64 sampling_number;                                ///< SamplingNumber
    HidGestureState state;
} HidGestureDummyStateAtomicStorage;

/// HidGestureLifo
typedef struct HidGestureLifo {
    HidCommonLifoHeader header;
    HidGestureDummyStateAtomicStorage storage[17];
} HidGestureLifo;

/// HidGestureSharedMemoryFormat
typedef struct HidGestureSharedMemoryFormat {
    HidGestureLifo lifo;
    u8 pad[0xF8];
} HidGestureSharedMemoryFormat;

// End HidGesture

/// HidConsoleSixAxisSensor
typedef struct {
    u64 sampling_number;                                ///< SamplingNumber
    u8 is_seven_six_axis_sensor_at_rest;                ///< IsSevenSixAxisSensorAtRest
    u8 pad[0x3];                                        ///< Padding
    float verticalization_error;                        ///< VerticalizationError
    UtilFloat3 gyro_bias;                               ///< GyroBias
    u8 pad2[0x4];                                       ///< Padding
} HidConsoleSixAxisSensor;

/// HidSharedMemory
typedef struct HidSharedMemory {
    u8 debug_pad[0x400];
    HidTouchScreenSharedMemoryFormat touchscreen;
    HidMouseSharedMemoryFormat mouse;
    HidKeyboardSharedMemoryFormat keyboard;
    u8 digitizer[0x1000];                               ///< [10.0.0+] Digitizer [1.0.0-9.2.0] BasicXpad
    u8 home_button[0x200];
    u8 sleep_button[0x200];
    u8 capture_button[0x200];
    u8 input_detector[0x800];
    u8 unique_pad[0x4000];                              ///< [1.0.0-4.1.0] UniquePad
    HidNpadSharedMemoryFormat npad;
    HidGestureSharedMemoryFormat gesture;
    HidConsoleSixAxisSensor console_six_axis_sensor;    ///< [5.0.0+] ConsoleSixAxisSensor
    u8 unk_x3C220[0x3DE0];
} HidSharedMemory;

/// HidSevenSixAxisSensorState
typedef struct {
    u64 timestamp0;
    u64 sampling_number;

    u64 unk_x10;
    float unk_x18[10];
} HidSevenSixAxisSensorState;

/// HidSevenSixAxisSensorStateEntry
typedef struct {
    u64 sampling_number;
    u64 unused;
    HidSevenSixAxisSensorState state;
} HidSevenSixAxisSensorStateEntry;

/// HidSevenSixAxisSensorStates
typedef struct {
    HidCommonLifoHeader header;
    HidSevenSixAxisSensorStateEntry storage[0x21];
} HidSevenSixAxisSensorStates;

/// HidSixAxisSensorHandle
typedef union HidSixAxisSensorHandle {
    u32 type_value;                                   ///< TypeValue
    struct {
        u32 npad_style_index : 8;                     ///< NpadStyleIndex
        u32 npad_id_type : 8;                         ///< PlayerNumber / NpadIdType
        u32 idx : 8;                                  ///< Idx
        u32 pad : 8;                                  ///< Padding
    };
} HidSixAxisSensorHandle;

/// HidVibrationDeviceHandle
typedef union HidVibrationDeviceHandle {
    u32 type_value;                                   ///< TypeValue
    struct {
        u32 npad_style_index : 8;                     ///< NpadStyleIndex
        u32 player_number : 8;                        ///< PlayerNumber
        u32 device_idx : 8;                           ///< DeviceIdx
        u32 pad : 8;                                  ///< Padding
    };
} HidVibrationDeviceHandle;

/// HidVibrationDeviceInfo
typedef struct HidVibrationDeviceInfo {
    u32 unk_x0;
    u32 unk_x4; ///< 0x1 for left-joycon, 0x2 for right-joycon.
} HidVibrationDeviceInfo;

/// HidVibrationValue
typedef struct HidVibrationValue {
    float amp_low;   ///< Low Band amplitude. 1.0f: Max amplitude.
    float freq_low;  ///< Low Band frequency in Hz.
    float amp_high;  ///< High Band amplitude. 1.0f: Max amplitude.
    float freq_high; ///< High Band frequency in Hz.
} HidVibrationValue;

static inline HidNpadIdType hidControllerIDToNpadIdType(HidControllerID id) {
    if (id <= CONTROLLER_PLAYER_8) return (HidNpadIdType)id;
    if (id == CONTROLLER_HANDHELD) return HidNpadIdType_Handheld;
    return HidNpadIdType_Other;//For CONTROLLER_UNKNOWN and invalid values return this.
}

static inline HidControllerID hidControllerIDFromNpadIdType(HidNpadIdType id) {
    if (id <= HidNpadIdType_No8) return (HidControllerID)id;
    if (id == HidNpadIdType_Handheld) return CONTROLLER_HANDHELD;
    return CONTROLLER_UNKNOWN;
}

/// Initialize hid. Called automatically during app startup.
Result hidInitialize(void);

/// Exit hid. Called automatically during app exit.
void hidExit(void);

void hidReset(void);

/// Gets the Service object for the actual hid service session.
Service* hidGetServiceSession(void);

/// Gets the address of the SharedMemory.
void* hidGetSharedmemAddr(void);

void hidScanInput(void);

void hidInitializeTouchScreen(void);
size_t hidGetTouchScreenStates(HidTouchScreenState *states, size_t count);

void hidInitializeMouse(void);
size_t hidGetMouseStates(HidMouseState *states, size_t count);

void hidInitializeKeyboard(void);
size_t hidGetKeyboardStates(HidKeyboardState *states, size_t count);

void hidInitializeNpad(void);

/// Gets a bitfield of \ref HidNpadStyleTag for the specified controller.
u32 hidGetNpadStyleSet(HidNpadIdType id);

/// Gets the \ref HidNpadJoyAssignmentMode for the specified controller.
HidNpadJoyAssignmentMode hidGetNpadJoyAssignment(HidNpadIdType id);

/// Gets the main \ref HidNpadControllerColor for the specified controller.
Result hidGetNpadControllerColorSingle(HidNpadIdType id, HidNpadControllerColor *color);

/// Gets the left/right \ref HidNpadControllerColor for the specified controller (Joy-Con pair in dual mode).
Result hidGetNpadControllerColorSplit(HidNpadIdType id, HidNpadControllerColor *color_left, HidNpadControllerColor *color_right);

/// Gets the bitfield of \ref HidDeviceTypeBits for the specified controller.
u32 hidGetNpadDeviceType(HidNpadIdType id);

/// Gets the \ref HidNpadSystemProperties for the specified controller.
void hidGetNpadSystemProperties(HidNpadIdType id, HidNpadSystemProperties *out);

/// Gets the \ref HidNpadSystemButtonProperties for the specified controller.
void hidGetNpadSystemButtonProperties(HidNpadIdType id, HidNpadSystemButtonProperties *out);

/// Gets the main \ref HidPowerInfo for the specified controller.
void hidGetNpadPowerInfoSingle(HidNpadIdType id, HidPowerInfo *info);

/// Gets the left/right \ref HidPowerInfo for the specified controller (Joy-Con pair in dual mode).
void hidGetNpadPowerInfoSplit(HidNpadIdType id, HidPowerInfo *info_left, HidPowerInfo *info_right);

/// Gets a bitfield of AppletFooterUiAttribute for the specified Npad.
/// Only available on [9.0.0+].
u32 hidGetAppletFooterUiAttributesSet(HidNpadIdType id);

/// Gets \ref HidAppletFooterUiType for the specified Npad.
/// Only available on [9.0.0+].
HidAppletFooterUiType hidGetAppletFooterUiTypes(HidNpadIdType id);

size_t hidGetNpadStatesFullKey(HidNpadIdType id, HidNpadFullKeyState *states, size_t count);
size_t hidGetNpadStatesHandheld(HidNpadIdType id, HidNpadHandheldState *states, size_t count);
size_t hidGetNpadStatesJoyDual(HidNpadIdType id, HidNpadJoyDualState *states, size_t count);
size_t hidGetNpadStatesJoyLeft(HidNpadIdType id, HidNpadJoyLeftState *states, size_t count);
size_t hidGetNpadStatesJoyRight(HidNpadIdType id, HidNpadJoyRightState *states, size_t count);
size_t hidGetNpadStatesGc(HidNpadIdType id, HidNpadGcState *states, size_t count);
size_t hidGetNpadStatesPalma(HidNpadIdType id, HidNpadPalmaState *states, size_t count);
size_t hidGetNpadStatesLark(HidNpadIdType id, HidNpadLarkState *states, size_t count);
size_t hidGetNpadStatesHandheldLark(HidNpadIdType id, HidNpadHandheldLarkState *states, size_t count);
size_t hidGetNpadStatesLucia(HidNpadIdType id, HidNpadLuciaState *states, size_t count);
size_t hidGetNpadStatesSystemExt(HidNpadIdType id, HidNpadSystemExtState *states, size_t count);
size_t hidGetNpadStatesSystem(HidNpadIdType id, HidNpadSystemState *states, size_t count);

size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle handle, HidSixAxisSensorState *states, size_t count);

void hidInitializeGesture(void);
size_t hidGetGestureStates(HidGestureState *states, size_t count);

bool hidIsControllerConnected(HidControllerID id);

u64 hidKeysHeld(HidControllerID id);
u64 hidKeysDown(HidControllerID id);
u64 hidKeysUp(HidControllerID id);

u64 hidMouseButtonsHeld(void);
u64 hidMouseButtonsDown(void);
u64 hidMouseButtonsUp(void);
void hidMouseRead(MousePosition *pos);
u32 hidMouseMultiRead(MousePosition *entries, u32 num_entries);

bool hidKeyboardModifierHeld(HidKeyboardModifier modifier);
bool hidKeyboardModifierDown(HidKeyboardModifier modifier);
bool hidKeyboardModifierUp(HidKeyboardModifier modifier);

bool hidKeyboardHeld(HidKeyboardScancode key);
bool hidKeyboardDown(HidKeyboardScancode key);
bool hidKeyboardUp(HidKeyboardScancode key);

u32 hidTouchCount(void);
void hidTouchRead(touchPosition *pos, u32 point_id);

void hidJoystickRead(JoystickPosition *pos, HidControllerID id, HidControllerJoystick stick);
u32 hidSixAxisSensorValuesRead(SixAxisSensorValues *values, HidControllerID id, u32 num_entries);

/// This can be used to check what CONTROLLER_P1_AUTO uses.
/// Returns 0 when CONTROLLER_PLAYER_1 is connected, otherwise returns 1 for handheld-mode.
bool hidGetHandheldMode(void);

/// SetSixAxisSensorFusionParameters. unk0 must be 0.0f-1.0f.
Result hidSetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float unk0, float unk1);

/// GetSixAxisSensorFusionParameters
Result hidGetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float *unk0, float *unk1);

/// ResetSixAxisSensorFusionParameters
Result hidResetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle);

/// Sets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
Result hidSetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode mode);

/// Gets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
Result hidGetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode *mode);

/// Resets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle to ::HidGyroscopeZeroDriftMode_Standard.
Result hidResetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle);

/// Sets which controller styles are supported, bitfield of \ref HidNpadStyleTag. This is automatically called with all styles in \ref hidInitialize.
Result hidSetSupportedNpadStyleSet(u32 style_set);

/// Gets which controller styles are supported, bitfield of \ref HidNpadStyleTag.
Result hidGetSupportedNpadStyleSet(u32 *style_set);

/// This is automatically called with HidNpadIdType_No{1-8} and HidNpadIdType_Handheld in \ref hidInitialize.
/// count must be <=10. Each entry in buf must be HidNpadIdType_No{1-8} or HidNpadIdType_Handheld.
Result hidSetSupportedNpadIdType(const HidNpadIdType *buf, size_t count);

/// Gets an event with the specified autoclear for the input controller.
/// The user *must* close the event when finished with it / before the app exits.
/// This is signaled when the \ref hidGetNpadStyleSet output is updated for the controller.
Result hidAcquireNpadStyleSetUpdateEventHandle(HidNpadIdType id, Event* out_event, bool autoclear);

/// Sets the hold-type, see \ref HidJoyHoldType.
Result hidSetNpadJoyHoldType(HidJoyHoldType type);

/// Gets the hold-type, see \ref HidJoyHoldType.
Result hidGetNpadJoyHoldType(HidJoyHoldType *type);

/// Use this if you want to use a single joy-con as a dedicated HidNpadIdType_No*.
/// When used, both joy-cons in a pair should be used with this (HidNpadIdType_No1 and HidNpadIdType_No2 for example).
/// id must be HidNpadIdType_No*.
Result hidSetNpadJoyAssignmentModeSingleByDefault(HidNpadIdType id);

/// Use this if you want to use a pair of joy-cons as a single HidNpadIdType_No*. Only necessary if you want to use this mode in your application after \ref hidSetNpadJoyAssignmentModeSingleByDefault was used with this pair of joy-cons.
/// Used automatically during app startup/exit for all controllers.
/// When used, both joy-cons in a pair should be used with this (HidNpadIdType_No1 and HidNpadIdType_No2 for example).
/// id must be HidNpadIdType_No*.
Result hidSetNpadJoyAssignmentModeDual(HidNpadIdType id);

/// Merge two single joy-cons into a dual-mode controller. Use this after \ref hidSetNpadJoyAssignmentModeDual, when \ref hidSetNpadJoyAssignmentModeSingleByDefault was previously used (this includes using this manually at application exit).
/// To be successful, id0/id1 must correspond to controllers supporting styles HidNpadStyleTag_NpadJoyLeft/Right, or HidNpadStyleTag_NpadJoyRight/Left.
/// If successful, the id of the resulting dual controller is set to id0.
Result hidMergeSingleJoyAsDualJoy(HidNpadIdType id0, HidNpadIdType id1);

Result hidInitializeVibrationDevices(HidVibrationDeviceHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

/// Gets HidVibrationDeviceInfo for the specified device.
Result hidGetVibrationDeviceInfo(HidVibrationDeviceHandle handle, HidVibrationDeviceInfo *VibrationDeviceInfo);

/// Send the VibrationValue to the specified device.
Result hidSendVibrationValue(HidVibrationDeviceHandle handle, HidVibrationValue *VibrationValue);

/// Gets the current HidVibrationValue for the specified device.
Result hidGetActualVibrationValue(HidVibrationDeviceHandle handle, HidVibrationValue *VibrationValue);

/// Sets whether vibration is allowed, this also affects the config displayed by System Settings.
Result hidPermitVibration(bool flag);

/// Gets whether vibration is allowed.
Result hidIsVibrationPermitted(bool *flag);

/// Send VibrationValues[index] to handles[index], where count is the number of entries in the handles/VibrationValues arrays.
Result hidSendVibrationValues(const HidVibrationDeviceHandle *handles, HidVibrationValue *VibrationValues, s32 count);

/// Gets whether vibration is available with the specified device. Only available on [7.0.0+].
Result hidIsVibrationDeviceMounted(HidVibrationDeviceHandle handle, bool *flag);

/// Gets SixAxisSensorHandles. total_handles==2 can only be used with ::HidNpadStyleTag_NpadJoyDual.
Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

/// Starts the SixAxisSensor for the specified handle.
Result hidStartSixAxisSensor(HidSixAxisSensorHandle handle);

/// Stops the SixAxisSensor for the specified handle.
Result hidStopSixAxisSensor(HidSixAxisSensorHandle handle);

/// Starts the SevenSixAxisSensor. Only available on [5.0.0+].
Result hidStartSevenSixAxisSensor(void);

/// Stops the SevenSixAxisSensor. Only available on [5.0.0+].
Result hidStopSevenSixAxisSensor(void);

/// Initializes the SevenSixAxisSensor. Only available on [5.0.0+].
Result hidInitializeSevenSixAxisSensor(void);

/// Finalizes the SevenSixAxisSensor. Also used automatically by \ref hidExit. Only available on [5.0.0+].
Result hidFinalizeSevenSixAxisSensor(void);

/// Sets the SevenSixAxisSensor FusionStrength. Only available on [5.0.0+].
Result hidSetSevenSixAxisSensorFusionStrength(float strength);

/// Gets the SevenSixAxisSensor FusionStrength. Only available on [5.0.0+].
Result hidGetSevenSixAxisSensorFusionStrength(float *strength);

/// Resets the timestamp for the SevenSixAxisSensor. Only available on [6.0.0+].
Result hidResetSevenSixAxisSensorTimestamp(void);

/// GetSevenSixAxisSensorStates. Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
Result hidGetSevenSixAxisSensorStates(HidSevenSixAxisSensorState *states, size_t count, size_t *total_out);

/// IsSevenSixAxisSensorAtRest. Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
Result hidIsSevenSixAxisSensorAtRest(bool *out);

/// GetSensorFusionError. Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
Result hidGetSensorFusionError(float *out);

/// GetGyroBias. Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
Result hidGetGyroBias(UtilFloat3 *out);

/// Gets the \ref HidNpadInterfaceType for the specified controller.
/// Only available on [4.0.0+].
Result hidGetNpadInterfaceType(HidNpadIdType id, u8 *out);

