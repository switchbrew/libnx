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

    HidNpadStyleSet_NpadFullCtrl = HidNpadStyleTag_NpadFullKey  | HidNpadStyleTag_NpadHandheld | HidNpadStyleTag_NpadJoyDual,  ///< Style set comprising Npad styles containing the full set of controls {FullKey, Handheld, JoyDual}
    HidNpadStyleSet_NpadStandard = HidNpadStyleSet_NpadFullCtrl | HidNpadStyleTag_NpadJoyLeft  | HidNpadStyleTag_NpadJoyRight, ///< Style set comprising all standard Npad styles {FullKey, Handheld, JoyDual, JoyLeft, JoyRight}
} HidNpadStyleTag;

/// HidColorAttribute
typedef enum {
    HidColorAttribute_Ok              = 0,            ///< Ok
    HidColorAttribute_ReadError       = 1,            ///< ReadError
    HidColorAttribute_NoController    = 2,            ///< NoController
} HidColorAttribute;

/// HidNpadButton
typedef enum {
    HidNpadButton_A             = BIT(0),  ///< A button / Right face button
    HidNpadButton_B             = BIT(1),  ///< B button / Down face button
    HidNpadButton_X             = BIT(2),  ///< X button / Up face button
    HidNpadButton_Y             = BIT(3),  ///< Y button / Left face button
    HidNpadButton_StickL        = BIT(4),  ///< Left Stick button
    HidNpadButton_StickR        = BIT(5),  ///< Right Stick button
    HidNpadButton_L             = BIT(6),  ///< L button
    HidNpadButton_R             = BIT(7),  ///< R button
    HidNpadButton_ZL            = BIT(8),  ///< ZL button
    HidNpadButton_ZR            = BIT(9),  ///< ZR button
    HidNpadButton_Plus          = BIT(10), ///< Plus button
    HidNpadButton_Minus         = BIT(11), ///< Minus button
    HidNpadButton_Left          = BIT(12), ///< D-Pad Left button
    HidNpadButton_Up            = BIT(13), ///< D-Pad Up button
    HidNpadButton_Right         = BIT(14), ///< D-Pad Right button
    HidNpadButton_Down          = BIT(15), ///< D-Pad Down button
    HidNpadButton_StickLLeft    = BIT(16), ///< Left Stick pseudo-button when moved Left
    HidNpadButton_StickLUp      = BIT(17), ///< Left Stick pseudo-button when moved Up
    HidNpadButton_StickLRight   = BIT(18), ///< Left Stick pseudo-button when moved Right
    HidNpadButton_StickLDown    = BIT(19), ///< Left Stick pseudo-button when moved Down
    HidNpadButton_StickRLeft    = BIT(20), ///< Right Stick pseudo-button when moved Left
    HidNpadButton_StickRUp      = BIT(21), ///< Right Stick pseudo-button when moved Up
    HidNpadButton_StickRRight   = BIT(22), ///< Right Stick pseudo-button when moved Right
    HidNpadButton_StickRDown    = BIT(23), ///< Right Stick pseudo-button when moved Left
    HidNpadButton_LeftSL        = BIT(24), ///< SL button on Left Joy-Con
    HidNpadButton_LeftSR        = BIT(25), ///< SR button on Left Joy-Con
    HidNpadButton_RightSL       = BIT(26), ///< SL button on Right Joy-Con
    HidNpadButton_RightSR       = BIT(27), ///< SR button on Right Joy-Con
    HidNpadButton_Palma         = BIT(28), ///< Top button on Poké Ball Plus (Palma) controller
    HidNpadButton_29            = BIT(29),
    HidNpadButton_HandheldLeftB = BIT(30), ///< B button on Left NES/HVC controller in Handheld mode

    HidNpadButton_AnyLeft  = HidNpadButton_Left   | HidNpadButton_StickLLeft  | HidNpadButton_StickRLeft,  ///< Bitmask containing all buttons that are considered Left (D-Pad, Sticks)
    HidNpadButton_AnyUp    = HidNpadButton_Up     | HidNpadButton_StickLUp    | HidNpadButton_StickRUp,    ///< Bitmask containing all buttons that are considered Up (D-Pad, Sticks)
    HidNpadButton_AnyRight = HidNpadButton_Right  | HidNpadButton_StickLRight | HidNpadButton_StickRRight, ///< Bitmask containing all buttons that are considered Right (D-Pad, Sticks)
    HidNpadButton_AnyDown  = HidNpadButton_Down   | HidNpadButton_StickLDown  | HidNpadButton_StickRDown,  ///< Bitmask containing all buttons that are considered Down (D-Pad, Sticks)
    HidNpadButton_AnySL    = HidNpadButton_LeftSL | HidNpadButton_RightSL,                                 ///< Bitmask containing SL buttons on both Joy-Cons (Left/Right)
    HidNpadButton_AnySR    = HidNpadButton_LeftSR | HidNpadButton_RightSR,                                 ///< Bitmask containing SR buttons on both Joy-Cons (Left/Right)
} HidNpadButton;

/// HidControllerKeys
typedef enum {
    KEY_A            = HidNpadButton_A,
    KEY_B            = HidNpadButton_B,
    KEY_X            = HidNpadButton_X,
    KEY_Y            = HidNpadButton_Y,
    KEY_LSTICK       = HidNpadButton_StickL,
    KEY_RSTICK       = HidNpadButton_StickR,
    KEY_L            = HidNpadButton_L,
    KEY_R            = HidNpadButton_R,
    KEY_ZL           = HidNpadButton_ZL,
    KEY_ZR           = HidNpadButton_ZR,
    KEY_PLUS         = HidNpadButton_Plus,
    KEY_MINUS        = HidNpadButton_Minus,
    KEY_DLEFT        = HidNpadButton_Left,
    KEY_DUP          = HidNpadButton_Up,
    KEY_DRIGHT       = HidNpadButton_Right,
    KEY_DDOWN        = HidNpadButton_Down,
    KEY_LSTICK_LEFT  = HidNpadButton_StickLLeft,
    KEY_LSTICK_UP    = HidNpadButton_StickLUp,
    KEY_LSTICK_RIGHT = HidNpadButton_StickLRight,
    KEY_LSTICK_DOWN  = HidNpadButton_StickLDown,
    KEY_RSTICK_LEFT  = HidNpadButton_StickRLeft,
    KEY_RSTICK_UP    = HidNpadButton_StickRUp,
    KEY_RSTICK_RIGHT = HidNpadButton_StickRRight,
    KEY_RSTICK_DOWN  = HidNpadButton_StickRDown,
    KEY_SL_LEFT      = HidNpadButton_LeftSL,
    KEY_SR_LEFT      = HidNpadButton_LeftSR,
    KEY_SL_RIGHT     = HidNpadButton_RightSL,
    KEY_SR_RIGHT     = HidNpadButton_RightSR,
    KEY_NES_HANDHELD_LEFT_B = HidNpadButton_HandheldLeftB,

    KEY_HOME         = BIT(18),      ///< HOME button, only available for use with HiddbgHdlsState::buttons.
    KEY_CAPTURE      = BIT(19),      ///< Capture button, only available for use with HiddbgHdlsState::buttons.
    KEY_TOUCH        = BIT(28),      ///< Pseudo-key for at least one finger on the touch screen

    KEY_JOYCON_RIGHT = HidNpadButton_A,
    KEY_JOYCON_DOWN  = HidNpadButton_B,
    KEY_JOYCON_UP    = HidNpadButton_X,
    KEY_JOYCON_LEFT  = HidNpadButton_Y,

    KEY_UP    = HidNpadButton_AnyUp,
    KEY_DOWN  = HidNpadButton_AnyDown,
    KEY_LEFT  = HidNpadButton_AnyLeft,
    KEY_RIGHT = HidNpadButton_AnyRight,
    KEY_SL    = HidNpadButton_AnySL,
    KEY_SR    = HidNpadButton_AnySR,
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

/// NpadJoyHoldType
typedef enum {
    HidNpadJoyHoldType_Vertical          = 0,       ///< Default / Joy-Con held vertically.
    HidNpadJoyHoldType_Horizontal        = 1,       ///< Joy-Con held horizontally.
} HidNpadJoyHoldType;

/// NpadJoyDeviceType
typedef enum {
    HidNpadJoyDeviceType_Left            = 0,       ///< Left
    HidNpadJoyDeviceType_Right           = 1,       ///< Right
} HidNpadJoyDeviceType;

/// This controls how many Joy-Cons must be attached for handheld-mode to be activated.
typedef enum {
    HidNpadHandheldActivationMode_Dual     = 0,     ///< Dual (2 Joy-Cons)
    HidNpadHandheldActivationMode_Single   = 1,     ///< Single (1 Joy-Con)
    HidNpadHandheldActivationMode_None     = 2,     ///< None (0 Joy-Cons)
} HidNpadHandheldActivationMode;

/// NpadJoyAssignmentMode
typedef enum {
    HidNpadJoyAssignmentMode_Dual   = 0,            ///< Dual (Set by \ref hidSetNpadJoyAssignmentModeDual)
    HidNpadJoyAssignmentMode_Single = 1,            ///< Single (Set by hidSetNpadJoyAssignmentModeSingle*())
} HidNpadJoyAssignmentMode;

/// NpadCommunicationMode
typedef enum {
    HidNpadCommunicationMode_Unknown0   = 0,        ///< Unknown
    HidNpadCommunicationMode_Unknown1   = 1,        ///< Unknown
    HidNpadCommunicationMode_Unknown2   = 2,        ///< Unknown
    HidNpadCommunicationMode_Unknown3   = 3,        ///< Unknown
} HidNpadCommunicationMode;

/// DeviceType (system)
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

/// AppletFooterUiType (system)
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

/// NpadInterfaceType (system)
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

/// Type values for HidVibrationDeviceInfo::type.
typedef enum {
    HidVibrationDeviceType_Unknown                          = 0,     ///< Unknown
    HidVibrationDeviceType_LinearResonantActuator           = 1,     ///< LinearResonantActuator
    HidVibrationDeviceType_GcErm                            = 2,     ///< GcErm (::HidNpadStyleTag_NpadGc)
} HidVibrationDeviceType;

/// VibrationDevicePosition
typedef enum {
    HidVibrationDevicePosition_None                         = 0,     ///< None
    HidVibrationDevicePosition_Left                         = 1,     ///< Left
    HidVibrationDevicePosition_Right                        = 2,     ///< Right
} HidVibrationDevicePosition;

/// VibrationGcErmCommand
typedef enum {
    HidVibrationGcErmCommand_Stop                           = 0,     ///< Stops the vibration with a decay phase.
    HidVibrationGcErmCommand_Start                          = 1,     ///< Starts the vibration.
    HidVibrationGcErmCommand_StopHard                       = 2,     ///< Stops the vibration immediately, with no decay phase.
} HidVibrationGcErmCommand;

/// touchPosition
typedef struct touchPosition {
    u32 id;
    u32 px;
    u32 py;
    u32 dx;
    u32 dy;
    u32 angle;
} touchPosition;

/// HidAnalogStickState
typedef struct HidAnalogStickState {
    s32 x;                                    ///< X
    s32 y;                                    ///< Y
} HidAnalogStickState;

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

/// HidDirectionState
typedef struct HidDirectionState {
    float direction[3][3];                      ///< 3x3 matrix
} HidDirectionState;

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
    u64 unused;                                 ///< Unused
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

/// HidTouchScreenConfigurationForNx
typedef struct {
    u8 config[0x10];                                 ///< Unknown
} HidTouchScreenConfigurationForNx;

// End HidTouchScreen

// Begin HidMouse

/// HidMouseState
typedef struct HidMouseState {
    u64 sampling_number;                        ///< SamplingNumber
    s32 x;                                      ///< X
    s32 y;                                      ///< Y
    s32 delta_x;                                ///< DeltaX
    s32 delta_y;                                ///< DeltaY
    s32 wheel_delta_x;                          ///< WheelDeltaX
    s32 wheel_delta_y;                          ///< WheelDeltaY
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
    u64 buttons;                                        ///< Bitfield of \ref HidNpadButton.
    HidAnalogStickState analog_stick_l;                 ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                 ///< AnalogStickR
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    u32 reserved;                                       ///< Reserved
} HidNpadCommonState;

typedef HidNpadCommonState HidNpadFullKeyState;         ///< State for ::HidNpadStyleTag_NpadFullKey.
typedef HidNpadCommonState HidNpadHandheldState;        ///< State for ::HidNpadStyleTag_NpadHandheld.
typedef HidNpadCommonState HidNpadJoyDualState;         ///< State for ::HidNpadStyleTag_NpadJoyDual.
typedef HidNpadCommonState HidNpadJoyLeftState;         ///< State for ::HidNpadStyleTag_NpadJoyLeft.
typedef HidNpadCommonState HidNpadJoyRightState;        ///< State for ::HidNpadStyleTag_NpadJoyRight.

/// State for ::HidNpadStyleTag_NpadGc. Loaded from the same lifo as \ref HidNpadFullKeyState, with the additional trigger_l/trigger_r loaded from elsewhere.
typedef struct HidNpadGcState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidNpadButton.
    HidAnalogStickState analog_stick_l;                 ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                 ///< AnalogStickR
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    u32 trigger_l;                                      ///< L analog trigger. Valid range: 0x0-0x7FFF.
    u32 trigger_r;                                      ///< R analog trigger. Valid range: 0x0-0x7FFF.
    u32 pad;
} HidNpadGcState;

typedef HidNpadCommonState HidNpadPalmaState;           ///< State for ::HidNpadStyleTag_NpadPalma.

/// State for ::HidNpadStyleTag_NpadLark. The base state is loaded from the same lifo as \ref HidNpadFullKeyState.
typedef struct HidNpadLarkState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidNpadButton.
    HidAnalogStickState analog_stick_l;                 ///< This is always zero.
    HidAnalogStickState analog_stick_r;                 ///< This is always zero.
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLarkType lark_type_l_and_main;               ///< \ref HidNpadLarkType LarkTypeLAndMain
} HidNpadLarkState;

/// State for ::HidNpadStyleTag_NpadHandheldLark. The base state is loaded from the same lifo as \ref HidNpadHandheldState.
typedef struct HidNpadHandheldLarkState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidNpadButton.
    HidAnalogStickState analog_stick_l;                 ///< AnalogStickL
    HidAnalogStickState analog_stick_r;                 ///< AnalogStickR
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLarkType lark_type_l_and_main;               ///< \ref HidNpadLarkType LarkTypeLAndMain
    HidNpadLarkType lark_type_r;                        ///< \ref HidNpadLarkType LarkTypeR
    u32 pad;
} HidNpadHandheldLarkState;

/// State for ::HidNpadStyleTag_NpadLucia. The base state is loaded from the same lifo as \ref HidNpadFullKeyState.
typedef struct HidNpadLuciaState {
    u64 sampling_number;                                ///< SamplingNumber
    u64 buttons;                                        ///< Bitfield of \ref HidNpadButton.
    HidAnalogStickState analog_stick_l;                 ///< This is always zero.
    HidAnalogStickState analog_stick_r;                 ///< This is always zero.
    u32 attributes;                                     ///< Bitfield of \ref HidNpadAttribute.
    HidNpadLuciaType lucia_type;                        ///< \ref HidNpadLuciaType
} HidNpadLuciaState;

typedef HidNpadCommonState HidNpadSystemExtState;       ///< State for ::HidNpadStyleTag_NpadSystemExt.
typedef HidNpadCommonState HidNpadSystemState;          ///< State for ::HidNpadStyleTag_NpadSystem. Analog-sticks state are always zero. Only the following button bits are available: HidNpadButton_A, HidNpadButton_B, HidNpadButton_X, HidNpadButton_Y, HidNpadButton_Left, HidNpadButton_Up, HidNpadButton_Right, HidNpadButton_Down, HidNpadButton_L, HidNpadButton_R.

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
    HidVector acceleration;                             ///< Acceleration
    HidVector angular_velocity;                         ///< AngularVelocity
    HidVector angle;                                    ///< Angle
    HidDirectionState direction;                        ///< Direction
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
    u64 is_charging : 3;                                             ///< Use \ref hidGetNpadPowerInfoSingle / \ref hidGetNpadPowerInfoSplit instead of accessing this directly.
    u64 is_powered : 3;                                              ///< Use \ref hidGetNpadPowerInfoSingle / \ref hidGetNpadPowerInfoSplit instead of accessing this directly.

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

/// HidPowerInfo (system)
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

    HidNpadCommonLifo full_key_lifo;             ///< FullKeyLifo
    HidNpadCommonLifo handheld_lifo;             ///< HandheldLifo
    HidNpadCommonLifo joy_dual_lifo;             ///< JoyDualLifo
    HidNpadCommonLifo joy_left_lifo;             ///< JoyLeftLifo
    HidNpadCommonLifo joy_right_lifo;            ///< JoyRightLifo
    HidNpadCommonLifo palma_lifo;                ///< PalmaLifo
    HidNpadCommonLifo system_ext_lifo;           ///< SystemExtLifo

    HidNpadSixAxisSensorLifo full_key_six_axis_sensor_lifo;                         ///< FullKeySixAxisSensorLifo
    HidNpadSixAxisSensorLifo handheld_six_axis_sensor_lifo;                         ///< HandheldSixAxisSensorLifo
    HidNpadSixAxisSensorLifo joy_dual_left_six_axis_sensor_lifo;                    ///< JoyDualLeftSixAxisSensorLifo
    HidNpadSixAxisSensorLifo joy_dual_right_six_axis_sensor_lifo;                   ///< JoyDualRightSixAxisSensorLifo
    HidNpadSixAxisSensorLifo joy_left_six_axis_sensor_lifo;                         ///< JoyLeftSixAxisSensorLifo
    HidNpadSixAxisSensorLifo joy_right_six_axis_sensor_lifo;                        ///< JoyRightSixAxisSensorLifo

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
    u32 type;                                         ///< \ref HidVibrationDeviceType
    u32 position;                                     ///< \ref HidVibrationDevicePosition
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

///@name TouchScreen
///@{

/// Initialize TouchScreen. Must be called when TouchScreen is being used. Used automatically by \ref hidScanInput when required.
void hidInitializeTouchScreen(void);

/**
 * @brief Gets \ref HidTouchScreenState.
 * @param[out] states Output array of \ref HidTouchScreenState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetTouchScreenStates(HidTouchScreenState *states, size_t count);

///@}

///@name Mouse
///@{

/// Initialize Mouse. Must be called when Mouse is being used. Used automatically by \ref hidScanInput when required.
void hidInitializeMouse(void);

/**
 * @brief Gets \ref HidMouseState.
 * @param[out] states Output array of \ref HidMouseState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetMouseStates(HidMouseState *states, size_t count);

///@}

///@name Keyboard
///@{

/// Initialize Keyboard. Must be called when Keyboard is being used. Used automatically by \ref hidScanInput when required.
void hidInitializeKeyboard(void);

/**
 * @brief Gets \ref HidKeyboardState.
 * @param[out] states Output array of \ref HidKeyboardState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetKeyboardStates(HidKeyboardState *states, size_t count);

///@}

///@name Npad
///@{

/// Initialize Npad. Must be called when Npad is being used. Used automatically by \ref hidScanInput when required.
void hidInitializeNpad(void);

/**
 * @brief Gets the StyleSet for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @return Bitfield of \ref HidNpadStyleTag.
 */
u32 hidGetNpadStyleSet(HidNpadIdType id);

/**
 * @brief Gets the \ref HidNpadJoyAssignmentMode for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @return \ref HidNpadJoyAssignmentMode
 */
HidNpadJoyAssignmentMode hidGetNpadJoyAssignment(HidNpadIdType id);

/**
 * @brief Gets the main \ref HidNpadControllerColor for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @param[out] color \ref HidNpadControllerColor
 */
Result hidGetNpadControllerColorSingle(HidNpadIdType id, HidNpadControllerColor *color);

/**
 * @brief Gets the left/right \ref HidNpadControllerColor for the specified Npad (Joy-Con pair in dual mode).
 * @param[in] id \ref HidNpadIdType
 * @param[out] color_left \ref HidNpadControllerColor
 * @param[out] color_right \ref HidNpadControllerColor
 */
Result hidGetNpadControllerColorSplit(HidNpadIdType id, HidNpadControllerColor *color_left, HidNpadControllerColor *color_right);

/**
 * @brief Gets the DeviceType for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @return Bitfield of \ref HidDeviceTypeBits.
 */
u32 hidGetNpadDeviceType(HidNpadIdType id);

/**
 * @brief Gets the \ref HidNpadSystemProperties for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @param[out] out \ref HidNpadSystemProperties
 */
void hidGetNpadSystemProperties(HidNpadIdType id, HidNpadSystemProperties *out);

/**
 * @brief Gets the \ref HidNpadSystemButtonProperties for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @param[out] out \ref HidNpadSystemButtonProperties
 */
void hidGetNpadSystemButtonProperties(HidNpadIdType id, HidNpadSystemButtonProperties *out);

/**
 * @brief Gets the main \ref HidPowerInfo for the specified Npad.
 * @param[in] id \ref HidNpadIdType
 * @param[out] info \ref HidPowerInfo
 */
void hidGetNpadPowerInfoSingle(HidNpadIdType id, HidPowerInfo *info);

/**
 * @brief Gets the left/right \ref HidPowerInfo for the specified Npad (Joy-Con pair in dual mode).
 * @param[in] id \ref HidNpadIdType
 * @param[out] info_left \ref HidPowerInfo
 * @param[out] info_right \ref HidPowerInfo
 */
void hidGetNpadPowerInfoSplit(HidNpadIdType id, HidPowerInfo *info_left, HidPowerInfo *info_right);

/**
 * @brief Gets the AppletFooterUiAttributesSet for the specified Npad.
 * @note Only available on [9.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @return Bitfield of AppletFooterUiAttribute (system).
 */
u32 hidGetAppletFooterUiAttributesSet(HidNpadIdType id);

/**
 * @brief Gets \ref HidAppletFooterUiType for the specified Npad.
 * @note Only available on [9.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @return \ref HidAppletFooterUiType
 */
HidAppletFooterUiType hidGetAppletFooterUiTypes(HidNpadIdType id);

/**
 * @brief Gets \ref HidNpadFullKeyState.
 * @param[out] states Output array of \ref HidNpadFullKeyState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesFullKey(HidNpadIdType id, HidNpadFullKeyState *states, size_t count);

/**
 * @brief Gets \ref HidNpadHandheldState.
 * @param[out] states Output array of \ref HidNpadHandheldState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesHandheld(HidNpadIdType id, HidNpadHandheldState *states, size_t count);

/**
 * @brief Gets \ref HidNpadJoyDualState.
 * @param[out] states Output array of \ref HidNpadJoyDualState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesJoyDual(HidNpadIdType id, HidNpadJoyDualState *states, size_t count);

/**
 * @brief Gets \ref HidNpadJoyLeftState.
 * @param[out] states Output array of \ref HidNpadJoyLeftState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesJoyLeft(HidNpadIdType id, HidNpadJoyLeftState *states, size_t count);

/**
 * @brief Gets \ref HidNpadJoyRightState.
 * @param[out] states Output array of \ref HidNpadJoyRightState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesJoyRight(HidNpadIdType id, HidNpadJoyRightState *states, size_t count);

/**
 * @brief Gets \ref HidNpadGcState.
 * @param[out] states Output array of \ref HidNpadGcState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesGc(HidNpadIdType id, HidNpadGcState *states, size_t count);

/**
 * @brief Gets \ref HidNpadPalmaState.
 * @param[out] states Output array of \ref HidNpadPalmaState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesPalma(HidNpadIdType id, HidNpadPalmaState *states, size_t count);

/**
 * @brief Gets \ref HidNpadLarkState.
 * @param[out] states Output array of \ref HidNpadLarkState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesLark(HidNpadIdType id, HidNpadLarkState *states, size_t count);

/**
 * @brief Gets \ref HidNpadHandheldLarkState.
 * @param[out] states Output array of \ref HidNpadHandheldLarkState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesHandheldLark(HidNpadIdType id, HidNpadHandheldLarkState *states, size_t count);

/**
 * @brief Gets \ref HidNpadLuciaState.
 * @param[out] states Output array of \ref HidNpadLuciaState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesLucia(HidNpadIdType id, HidNpadLuciaState *states, size_t count);

/**
 * @brief Gets \ref HidNpadSystemExtState.
 * @param[out] states Output array of \ref HidNpadSystemExtState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesSystemExt(HidNpadIdType id, HidNpadSystemExtState *states, size_t count);

/**
 * @brief Gets \ref HidNpadSystemState.
 * @param[out] states Output array of \ref HidNpadSystemState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetNpadStatesSystem(HidNpadIdType id, HidNpadSystemState *states, size_t count);

/**
 * @brief Gets \ref HidSixAxisSensorState for the specified handle.
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] states Output array of \ref HidSixAxisSensorState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetSixAxisSensorStates(HidSixAxisSensorHandle handle, HidSixAxisSensorState *states, size_t count);

///@}

///@name Gesture
///@{

/// Initialize Gesture. Must be called when Gesture is being used.
void hidInitializeGesture(void);

/**
 * @brief Gets \ref HidGestureState.
 * @param[out] states Output array of \ref HidGestureState.
 * @param[in] count Size of the states array in entries.
 * @return Total output entries.
 */
size_t hidGetGestureStates(HidGestureState *states, size_t count);

///@}

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

/**
 * @brief This can be used to check what CONTROLLER_P1_AUTO uses.
 * @return 0 when CONTROLLER_PLAYER_1 is connected, otherwise returns 1 for handheld-mode.
 */
bool hidGetHandheldMode(void);

/**
 * @brief SendKeyboardLockKeyEvent
 * @note Same as \ref hidsysSendKeyboardLockKeyEvent.
 * @note Only available on [6.0.0+].
 * @param[in] events Bitfield of KeyboardLockKeyEvent.
 */
Result hidSendKeyboardLockKeyEvent(u32 events);

/**
 * @brief Gets SixAxisSensorHandles.
 * @note Only ::HidNpadStyleTag_NpadJoyDual supports total_handles==2.
 * @param[out] handles Output array of \ref HidSixAxisSensorHandle.
 * @param[in] total_handles Total handles for the handles array. Must be 1 or 2, if 2 handles aren't supported by the specified style an error is thrown.
 * @param[in] id \ref HidNpadIdType
 * @param[in] style \ref HidNpadStyleTag
 */
Result hidGetSixAxisSensorHandles(HidSixAxisSensorHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

/**
 * @brief Starts the SixAxisSensor for the specified handle.
 * @param[in] handle \ref HidSixAxisSensorHandle
 */
Result hidStartSixAxisSensor(HidSixAxisSensorHandle handle);

/**
 * @brief Stops the SixAxisSensor for the specified handle.
 * @param[in] handle \ref HidSixAxisSensorHandle
 */
Result hidStopSixAxisSensor(HidSixAxisSensorHandle handle);

/**
 * @brief IsSixAxisSensorFusionEnabled
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] out Output flag.
 */
Result hidIsSixAxisSensorFusionEnabled(HidSixAxisSensorHandle handle, bool *out);

/**
 * @brief EnableSixAxisSensorFusion
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[in] flag Flag
 */
Result hidEnableSixAxisSensorFusion(HidSixAxisSensorHandle handle, bool flag);

/**
 * @brief SetSixAxisSensorFusionParameters
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[in] unk0 Must be 0.0f-1.0f.
 * @param[in] unk1 Unknown
 */
Result hidSetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float unk0, float unk1);

/**
 * @brief GetSixAxisSensorFusionParameters
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] unk0 Unknown
 * @param[out] unk1 Unknown
 */
Result hidGetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle, float *unk0, float *unk1);

/**
 * @brief ResetSixAxisSensorFusionParameters
 * @param[in] handle \ref HidSixAxisSensorHandle
 */
Result hidResetSixAxisSensorFusionParameters(HidSixAxisSensorHandle handle);

/**
 * @brief Sets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[in] mode \ref HidGyroscopeZeroDriftMode
 */
Result hidSetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode mode);

/**
 * @brief Gets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] mode \ref HidGyroscopeZeroDriftMode
 */
Result hidGetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle, HidGyroscopeZeroDriftMode *mode);

/**
 * @brief Resets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle to ::HidGyroscopeZeroDriftMode_Standard.
 * @param[in] handle \ref HidSixAxisSensorHandle
 */
Result hidResetGyroscopeZeroDriftMode(HidSixAxisSensorHandle handle);

/**
 * @brief IsSixAxisSensorAtRest
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] out Output flag.
 */
Result hidIsSixAxisSensorAtRest(HidSixAxisSensorHandle handle, bool *out);

/**
 * @brief IsFirmwareUpdateAvailableForSixAxisSensor
 * @note Only available on [6.0.0+].
 * @param[in] handle \ref HidSixAxisSensorHandle
 * @param[out] out Output flag.
 */
Result hidIsFirmwareUpdateAvailableForSixAxisSensor(HidSixAxisSensorHandle handle, bool *out);

/**
 * @brief Sets which controller styles are supported.
 * @note This is automatically called with the needed styles in \ref hidScanInput when required.
 * @param[in] style_set Bitfield of \ref HidNpadStyleTag.
 */
Result hidSetSupportedNpadStyleSet(u32 style_set);

/**
 * @brief Gets which controller styles are supported.
 * @param[out] style_set Bitfield of \ref HidNpadStyleTag.
 */
Result hidGetSupportedNpadStyleSet(u32 *style_set);

/**
 * @brief Sets which \ref HidNpadIdType are supported.
 * @note This is automatically called with HidNpadIdType_No{1-8} and HidNpadIdType_Handheld when required in \ref hidScanInput.
 * @param[in] ids Input array of \ref HidNpadIdType.
 * @param[in] count Total entries in the ids array. Must be <=10.
 */
Result hidSetSupportedNpadIdType(const HidNpadIdType *ids, size_t count);

/**
 * @brief Gets an Event which is signaled when the \ref hidGetNpadStyleSet output is updated for the specified controller.
 * @note The Event must be closed by the user once finished with it.
 * @param[in] id \ref HidNpadIdType
 * @param[out] out_event Output Event.
 * @param[in] autoclear The autoclear for the Event.
**/
Result hidAcquireNpadStyleSetUpdateEventHandle(HidNpadIdType id, Event* out_event, bool autoclear);

/**
 * @brief DisconnectNpad
 * @param[in] id \ref HidNpadIdType
 */
Result hidDisconnectNpad(HidNpadIdType id);

/**
 * @brief GetPlayerLedPattern
 * @param[in] id \ref HidNpadIdType
 * @param[out] out Output value.
 */
Result hidGetPlayerLedPattern(HidNpadIdType id, u8 *out);

/**
 * @brief Sets the \ref HidNpadJoyHoldType.
 * @note Used automatically by \ref hidScanInput when required.
 * @param[in] type \ref HidNpadJoyHoldType
 */
Result hidSetNpadJoyHoldType(HidNpadJoyHoldType type);

/**
 * @brief Gets the \ref HidNpadJoyHoldType.
 * @param[out] type \ref HidNpadJoyHoldType
 */
Result hidGetNpadJoyHoldType(HidNpadJoyHoldType *type);

/**
 * @brief This is the same as \ref hidSetNpadJoyAssignmentModeSingle, except ::HidNpadJoyDeviceType_Left is used for the type.
 * @param[in] id \ref HidNpadIdType, must be HidNpadIdType_No*.
 */
Result hidSetNpadJoyAssignmentModeSingleByDefault(HidNpadIdType id);

/**
 * @brief This is the same as \ref hidSetNpadJoyAssignmentModeSingleWithDestination, except without the output params.
 * @param[in] id \ref HidNpadIdType, must be HidNpadIdType_No*.
 * @param[in] type \ref HidNpadJoyDeviceType
 */
Result hidSetNpadJoyAssignmentModeSingle(HidNpadIdType id, HidNpadJoyDeviceType type);

/**
 * @brief Use this if you want to use a pair of joy-cons as a single HidNpadIdType_No*. When used, both joy-cons in a pair should be used with this (HidNpadIdType_No1 and HidNpadIdType_No2 for example).
 * @note Used automatically by \ref hidScanInput when required.
 * @param[in] id \ref HidNpadIdType, must be HidNpadIdType_No*.
 */
Result hidSetNpadJoyAssignmentModeDual(HidNpadIdType id);

/**
 * @brief Merge two single joy-cons into a dual-mode controller. Use this after \ref hidSetNpadJoyAssignmentModeDual, when \ref hidSetNpadJoyAssignmentModeSingleByDefault was previously used (this includes using this manually at application exit).
 * @brief To be successful, id0/id1 must correspond to controllers supporting styles HidNpadStyleTag_NpadJoyLeft/Right, or HidNpadStyleTag_NpadJoyRight/Left.
 * @brief If successful, the id of the resulting dual controller is set to id0.
 * @param[in] id0 \ref HidNpadIdType
 * @param[in] id1 \ref HidNpadIdType
 */
Result hidMergeSingleJoyAsDualJoy(HidNpadIdType id0, HidNpadIdType id1);

/**
 * @brief StartLrAssignmentMode
 */
Result hidStartLrAssignmentMode(void);

/**
 * @brief StopLrAssignmentMode
 */
Result hidStopLrAssignmentMode(void);

/**
 * @brief Sets the \ref HidNpadHandheldActivationMode.
 * @param[in] mode \ref HidNpadHandheldActivationMode
 */
Result hidSetNpadHandheldActivationMode(HidNpadHandheldActivationMode mode);

/**
 * @brief Gets the \ref HidNpadHandheldActivationMode.
 * @param[out] out \ref HidNpadHandheldActivationMode
 */
Result hidGetNpadHandheldActivationMode(HidNpadHandheldActivationMode *out);

/**
 * @brief SwapNpadAssignment
 * @param[in] id0 \ref HidNpadIdType
 * @param[in] id1 \ref HidNpadIdType
 */
Result hidSwapNpadAssignment(HidNpadIdType id0, HidNpadIdType id1);

/**
 * @brief EnableUnintendedHomeButtonInputProtection
 * @note To get the state of this, use \ref hidGetNpadSystemButtonProperties to access HidNpadSystemButtonProperties::is_unintended_home_button_input_protection_enabled.
 * @param[in] id \ref HidNpadIdType
 * @param[in] flag Whether UnintendedHomeButtonInputProtection is enabled.
 */
Result hidEnableUnintendedHomeButtonInputProtection(HidNpadIdType id, bool flag);

/**
 * @brief Use this if you want to use a single joy-con as a dedicated HidNpadIdType_No*. When used, both joy-cons in a pair should be used with this (HidNpadIdType_No1 and HidNpadIdType_No2 for example).
 * @note Only available on [5.0.0+].
 * @param[in] id \ref HidNpadIdType, must be HidNpadIdType_No*.
 * @param[in] type \ref HidNpadJoyDeviceType
 * @param[out] flag Whether the dest output is set.
 * @param[out] dest \ref HidNpadIdType
 */
Result hidSetNpadJoyAssignmentModeSingleWithDestination(HidNpadIdType id, HidNpadJoyDeviceType type, bool *flag, HidNpadIdType *dest);

/**
 * @brief SetNpadAnalogStickUseCenterClamp
 * @note Only available on [6.1.0+].
 * @param[in] flag Flag
 */
Result hidSetNpadAnalogStickUseCenterClamp(bool flag);

/**
 * @brief Assigns the button(s) which trigger the CaptureButton.
 * @note Only available on [8.0.0+].
 * @param[in] style \ref HidNpadStyleTag, exactly 1 bit must be set.
 * @param[in] buttons Bitfield of \ref HidNpadButton, multiple bits can be set.
 */
Result hidSetNpadCaptureButtonAssignment(HidNpadStyleTag style, u64 buttons);

/**
 * @brief ClearNpadCaptureButtonAssignment
 * @note Only available on [8.0.0+].
 */
Result hidClearNpadCaptureButtonAssignment(void);

/**
 * @brief Gets and initializes vibration handles.
 * @note Only the following styles support total_handles 2: ::HidNpadStyleTag_NpadFullKey, ::HidNpadStyleTag_NpadHandheld, ::HidNpadStyleTag_NpadJoyDual, ::HidNpadStyleTag_NpadHandheldLark, ::HidNpadStyleTag_NpadSystem, ::HidNpadStyleTag_NpadSystemExt.
 * @param[out] handles Output array of \ref HidVibrationDeviceHandle.
 * @param[in] total_handles Total handles for the handles array. Must be 1 or 2, if 2 handles aren't supported by the specified style an error is thrown.
 * @param[in] id \ref HidNpadIdType
 * @param[in] style \ref HidNpadStyleTag
 */
Result hidInitializeVibrationDevices(HidVibrationDeviceHandle *handles, s32 total_handles, HidNpadIdType id, HidNpadStyleTag style);

/**
 * @brief Gets \ref HidVibrationDeviceInfo for the specified device.
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[out] out \ref HidVibrationDeviceInfo
 */
Result hidGetVibrationDeviceInfo(HidVibrationDeviceHandle handle, HidVibrationDeviceInfo *out);

/**
 * @brief Sends the \ref HidVibrationDeviceHandle to the specified device.
 * @note With ::HidVibrationDeviceType_GcErm, use \ref hidSendVibrationGcErmCommand instead.
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[in] value \ref HidVibrationValue
 */
Result hidSendVibrationValue(HidVibrationDeviceHandle handle, const HidVibrationValue *value);

/**
 * @brief Gets the current \ref HidVibrationValue for the specified device.
 * @note With ::HidVibrationDeviceType_GcErm, use \ref hidGetActualVibrationGcErmCommand instead.
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[out] out \ref HidVibrationValue
 */
Result hidGetActualVibrationValue(HidVibrationDeviceHandle handle, HidVibrationValue *out);

/**
 * @brief Sets whether vibration is allowed, this also affects the config displayed by System Settings.
 * @param[in] flag Flag
 */
Result hidPermitVibration(bool flag);

/**
 * @brief Gets whether vibration is allowed.
 * @param[out] flag Flag
 */
Result hidIsVibrationPermitted(bool *flag);

/**
 * @brief Send vibration values[index] to handles[index].
 * @note With ::HidVibrationDeviceType_GcErm, use \ref hidSendVibrationGcErmCommand instead.
 * @param[in] handles Input array of \ref HidVibrationDeviceHandle.
 * @param[in] values Input array of \ref HidVibrationValue.
 * @param[in] count Total entries in the handles/values arrays.
 */
Result hidSendVibrationValues(const HidVibrationDeviceHandle *handles, const HidVibrationValue *values, s32 count);

/**
 * @brief Send \ref HidVibrationGcErmCommand to the specified device, for ::HidVibrationDeviceType_GcErm.
 * @note Only available on [4.0.0+].
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[in] cmd \ref HidVibrationGcErmCommand
 */
Result hidSendVibrationGcErmCommand(HidVibrationDeviceHandle handle, HidVibrationGcErmCommand cmd);

/**
 * @brief Get \ref HidVibrationGcErmCommand for the specified device, for ::HidVibrationDeviceType_GcErm.
 * @note Only available on [4.0.0+].
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[out] out \ref HidVibrationGcErmCommand
 */
Result hidGetActualVibrationGcErmCommand(HidVibrationDeviceHandle handle, HidVibrationGcErmCommand *out);

/**
 * @brief Begins a forced-permitted vibration session.
 * @note Only available on [4.0.0+].
 */
Result hidBeginPermitVibrationSession(void);

/**
 * @brief Ends the session started by \ref hidBeginPermitVibrationSession.
 * @note Only available on [4.0.0+].
 */
Result hidEndPermitVibrationSession(void);

/**
 * @brief Gets whether vibration is available with the specified device.
 * @note Only available on [7.0.0+].
 * @param[in] handle \ref HidVibrationDeviceHandle
 * @param[out] flag Flag
 */
Result hidIsVibrationDeviceMounted(HidVibrationDeviceHandle handle, bool *flag);

/**
 * @brief Starts the SevenSixAxisSensor.
 * @note Only available on [5.0.0+].
 */
Result hidStartSevenSixAxisSensor(void);

/**
 * @brief Stops the SevenSixAxisSensor.
 * @note Only available on [5.0.0+].
 */
Result hidStopSevenSixAxisSensor(void);

/**
 * @brief Initializes the SevenSixAxisSensor.
 * @note Only available on [5.0.0+].
 */
Result hidInitializeSevenSixAxisSensor(void);

/**
 * @brief Finalizes the SevenSixAxisSensor.
 * @note This must be called before \ref hidExit.
 * @note Only available on [5.0.0+].
 */
Result hidFinalizeSevenSixAxisSensor(void);

/**
 * @brief Sets the SevenSixAxisSensor FusionStrength.
 * @note Only available on [5.0.0+].
 * @param[in] strength Strength
 */
Result hidSetSevenSixAxisSensorFusionStrength(float strength);

/**
 * @brief Gets the SevenSixAxisSensor FusionStrength.
 * @note Only available on [5.0.0+].
 * @param[out] strength Strength
 */
Result hidGetSevenSixAxisSensorFusionStrength(float *strength);

/**
 * @brief Resets the timestamp for the SevenSixAxisSensor.
 * @note Only available on [6.0.0+].
 */
Result hidResetSevenSixAxisSensorTimestamp(void);

/**
 * @brief GetSevenSixAxisSensorStates
 * @note Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
 * @param[out] states Output array of \ref HidSevenSixAxisSensorState.
 * @param[in] count Size of the states array in entries.
 * @param[out] total_out Total output entries.
 */
Result hidGetSevenSixAxisSensorStates(HidSevenSixAxisSensorState *states, size_t count, size_t *total_out);

/**
 * @brief IsSevenSixAxisSensorAtRest
 * @note Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
 * @param[out] out Output flag.
 */
Result hidIsSevenSixAxisSensorAtRest(bool *out);

/**
 * @brief GetSensorFusionError
 * @note Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
 * @param[out] out Output data.
 */
Result hidGetSensorFusionError(float *out);

/**
 * @brief GetGyroBias
 * @note Only available when \ref hidInitializeSevenSixAxisSensor was previously used.
 * @param[out] out \ref UtilFloat3
 */
Result hidGetGyroBias(UtilFloat3 *out);

/**
 * @brief IsUsbFullKeyControllerEnabled
 * @note Only available on [3.0.0+].
 * @param[out] out Output flag.
 */
Result hidIsUsbFullKeyControllerEnabled(bool *out);

/**
 * @brief EnableUsbFullKeyController
 * @note Only available on [3.0.0+].
 * @param[in] flag Flag
 */
Result hidEnableUsbFullKeyController(bool flag);

/**
 * @brief IsUsbFullKeyControllerConnected
 * @note Only available on [3.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out Output flag.
 */
Result hidIsUsbFullKeyControllerConnected(HidNpadIdType id, bool *out);

/**
 * @brief Gets the \ref HidNpadInterfaceType for the specified controller.
 * @note When available, \ref hidsysGetNpadInterfaceType should be used instead.
 * @note Only available on [4.0.0+].
 * @param[in] id \ref HidNpadIdType
 * @param[out] out \ref HidNpadInterfaceType
 */
Result hidGetNpadInterfaceType(HidNpadIdType id, u8 *out);

/**
 * @brief GetNpadOfHighestBatteryLevel
 * @note Only available on [10.0.0+].
 * @param[in] ids Input array of \ref HidNpadIdType, ::HidNpadIdType_Handheld is ignored.
 * @param[in] count Total entries in the ids array.
 * @param[out] out \ref HidNpadIdType
 */
Result hidGetNpadOfHighestBatteryLevel(const HidNpadIdType *ids, size_t count, HidNpadIdType *out);

/**
 * @brief SetNpadCommunicationMode
 * @param[in] mode \ref HidNpadCommunicationMode
 */
Result hidSetNpadCommunicationMode(HidNpadCommunicationMode mode);

/**
 * @brief GetNpadCommunicationMode
 * @param[out] out \ref HidNpadCommunicationMode
 */
Result hidGetNpadCommunicationMode(HidNpadCommunicationMode *out);

/**
 * @brief SetTouchScreenConfiguration
 * @note Only available on [9.0.0+].
 * @param[in] config \ref HidTouchScreenConfigurationForNx
 */
Result hidSetTouchScreenConfiguration(const HidTouchScreenConfigurationForNx *config);

/**
 * @brief IsFirmwareUpdateNeededForNotification
 * @note Only available on [9.0.0+].
 * @param[out] out Output flag.
 */
Result hidIsFirmwareUpdateNeededForNotification(bool *out);

