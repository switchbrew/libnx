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
    MOUSE_LEFT    = BIT(0),
    MOUSE_RIGHT   = BIT(1),
    MOUSE_MIDDLE  = BIT(2),
    MOUSE_FORWARD = BIT(3),
    MOUSE_BACK    = BIT(4),
} HidMouseButton;

/// HidKeyboardModifier
typedef enum {
    KBD_MOD_LCTRL      = BIT(0),
    KBD_MOD_LSHIFT     = BIT(1),
    KBD_MOD_LALT       = BIT(2),
    KBD_MOD_LMETA      = BIT(3),
    KBD_MOD_RCTRL      = BIT(4),
    KBD_MOD_RSHIFT     = BIT(5),
    KBD_MOD_RALT       = BIT(6),
    KBD_MOD_RMETA      = BIT(7),
    KBD_MOD_CAPSLOCK   = BIT(8),
    KBD_MOD_SCROLLLOCK = BIT(9),
    KBD_MOD_NUMLOCK    = BIT(10),
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

/// HidControllerColorDescription
typedef enum {
    COLORS_NONEXISTENT = BIT(1),
} HidControllerColorDescription;

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

/// HidControllerConnectionState
typedef enum {
    CONTROLLER_STATE_CONNECTED = BIT(0),
    CONTROLLER_STATE_WIRED     = BIT(1),
} HidControllerConnectionState;

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
    CONTROLLER_P1_AUTO = 10, ///< Not an actual HID-sysmodule ID. Only for hidKeys*()/hidJoystickRead()/hidSixAxisSensorValuesRead()/hidGetControllerColors()/hidIsControllerConnected(). Automatically uses CONTROLLER_PLAYER_1 when connected, otherwise uses CONTROLLER_HANDHELD.
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

/// HidCommonStateHeader
typedef struct HidCommonStateHeader {
    u64 timestamp_ticks;
    u64 total_entries;
    u64 latest_entry;
    u64 max_entry;
} HidCommonStateHeader;

// Begin HidTouchScreen

/// HidTouchScreenHeader
typedef struct HidTouchScreenHeader {
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
    u64 timestamp;
} HidTouchScreenHeader;

/// HidTouchScreenEntryHeader
typedef struct HidTouchScreenEntryHeader {
    u64 timestamp;
    u64 numTouches;
} HidTouchScreenEntryHeader;

/// HidTouchScreenEntryTouch
typedef struct HidTouchScreenEntryTouch {
    u64 timestamp;
    u32 padding;
    u32 touchIndex;
    u32 x;
    u32 y;
    u32 diameterX;
    u32 diameterY;
    u32 angle;
    u32 padding_2;
} HidTouchScreenEntryTouch;

/// HidTouchScreenEntry
typedef struct HidTouchScreenEntry {
    HidTouchScreenEntryHeader header;
    HidTouchScreenEntryTouch touches[16];
    u64 unk;
} HidTouchScreenEntry;

/// HidTouchScreen
typedef struct HidTouchScreen {
    HidTouchScreenHeader header;
    HidTouchScreenEntry entries[17];
    u8 padding[0x3c0];
} HidTouchScreen;

// End HidTouchScreen

// Begin HidMouse

/// HidMouseEntry
typedef struct HidMouseEntry {
    u64 timestamp;
    u64 timestamp_2;
    MousePosition position;
    u64 buttons;
} HidMouseEntry;

/// HidMouse
typedef struct HidMouse {
    HidCommonStateHeader header;
    HidMouseEntry entries[17];
    u8 padding[0xB0];
} HidMouse;

// End HidMouse

// Begin HidKeyboard

/// HidKeyboardEntry
typedef struct HidKeyboardEntry {
    u64 timestamp;
    u64 timestamp_2;
    u64 modifier;
    u32 keys[8];
} HidKeyboardEntry;

/// HidKeyboard
typedef struct HidKeyboard {
    HidCommonStateHeader header;
    HidKeyboardEntry entries[17];
    u8 padding[0x28];
} HidKeyboard;

// End HidKeyboard

// Begin HidNpad

/// Npad colors.
/// Color fields are zero when not set.
typedef struct HidNpadControllerColor
{
    u32 color_body;    ///< RGBA Body Color
    u32 color_buttons; ///< RGBA Buttons Color
} HidNpadControllerColor;

/// HidNpadStateHeader
typedef struct HidNpadStateHeader {
    u32 style_set;
    u32 npad_joy_assignment_mode;
    u32 single_colors_descriptor;
    HidNpadControllerColor single_colors;
    u32 split_colors_descriptor;
    HidNpadControllerColor left_colors;
    HidNpadControllerColor right_colors;
} HidNpadStateHeader;

/// HidNpadStateEntry
typedef struct HidNpadStateEntry {
    u64 timestamp;
    u64 buttons;
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];
    u32 connectionState;
    u32 pad;
} HidNpadStateEntry;

typedef HidNpadStateEntry HidNpadFullKeyState;
typedef HidNpadStateEntry HidNpadHandheldState;
typedef HidNpadStateEntry HidNpadJoyDualState;
typedef HidNpadStateEntry HidNpadJoyLeftState;
typedef HidNpadStateEntry HidNpadJoyRightState;
typedef HidNpadStateEntry HidNpadSystemExtState;
typedef HidNpadStateEntry HidNpadSystemState; ///< Joysticks state are always zero. Only the following button bits are available: KEY_A, KEY_B, KEY_X, KEY_Y, KEY_DLEFT, KEY_DUP, KEY_DRIGHT, KEY_DDOWN, KEY_L, KEY_R.

/// HidControllerInputEntry
typedef struct HidControllerInputEntry {
    u64 timestamp;
    HidNpadStateEntry state;
} HidControllerInputEntry;

/// HidControllerLayout
typedef struct HidControllerLayout {
    HidCommonStateHeader header;
    HidControllerInputEntry entries[17];
} HidControllerLayout;

/// HidNpadSixAxisSensorState
typedef struct HidNpadSixAxisSensorState {
    u64 timestamp;
    u64 unk_1;
    u64 timestamp_2;
    SixAxisSensorValues values;
    u64 unk_3;
} HidNpadSixAxisSensorState;

/// HidControllerSixAxisLayout
typedef struct HidControllerSixAxisLayout {
    HidCommonStateHeader header;
    HidNpadSixAxisSensorState entries[17];
} HidControllerSixAxisLayout;

/// NpadSystemProperties
typedef struct {
    u32 powerInfo : 6;                                    ///< Use \ref hidGetControllerPowerInfo instead of accessing this directly.

    u32 bit6 : 1;                                         ///< Unused
    u32 bit7 : 1;                                         ///< Unused
    u32 bit8 : 1;                                         ///< Unused
    u32 unsupportedButtonPressed_NpadSystem : 1;          ///< Unsupported button pressed with controller NpadSystem.
    u32 unsupportedButtonPressed_NpadSystemExt : 1;       ///< Unsupported button pressed with controller NpadSystemExt.

    u32 abxyButtonOriented : 1;
    u32 slSrButtonOriented : 1;
    u32 plusButtonCapability : 1;                         ///< [4.0.0+]
    u32 minusButtonCapability : 1;                        ///< [4.0.0+]
    u32 directionalButtonsSupported : 1;                  ///< [8.0.0+]

    u32 unused;
} HidNpadSystemProperties;

/// NpadSystemButtonProperties
typedef struct {
    u32 unintendedHomeButtonInputProtectionDisabled : 1;
} HidNpadSystemButtonProperties;

/// HidPowerInfo
typedef struct {
    bool powerConnected;
    bool isCharging;
    u32 batteryCharge;    ///< Battery charge, always 0-4.
} HidPowerInfo;

/// HidNfcXcdDeviceHandleState
typedef struct HidNfcXcdDeviceHandleState {
    u64 handle;
    u8 flag0;
    u8 flag1;
    u8 pad[6];
    u64 timestamp;
} HidNfcXcdDeviceHandleState;

/// HidNfcXcdDeviceHandleStateEntry
typedef struct HidNfcXcdDeviceHandleStateEntry {
    u64 timestamp;
    HidNfcXcdDeviceHandleState state;
} HidNfcXcdDeviceHandleStateEntry;

/// HidNpad
typedef struct HidNpad {
    HidNpadStateHeader header;
    HidControllerLayout layouts[7];
    HidControllerSixAxisLayout sixaxis[6];
    u32 deviceType;
    u32 pad;
    HidNpadSystemProperties system_properties;
    HidNpadSystemButtonProperties system_button_properties;
    u32 batteryCharge[3];
    union {
        struct { // [1.0.0-3.0.2]
            u8 nfc_xcd_device_handle_header[0x20];
            HidNfcXcdDeviceHandleStateEntry nfc_xcd_device_handle_state[2];
        };

        struct {
            u32 applet_footer_ui_attribute;
            u8 applet_footer_ui_type;
            u8 unk_x41AD[0x5B];
        };
    };
    u8 unk_2[0xDF8];
} HidNpad;

// End HidNpad

/// HidConsoleSixAxisSensor
typedef struct {
    u64 timestamp;                   ///< Timestamp in samples
    u8 is_at_rest;                   ///< IsSevenSixAxisSensorAtRest
    u8 pad[0x3];
    float verticalization_error;     ///< VerticalizationError
    UtilFloat3 gyro_bias;            ///< GyroBias
    u8 pad2[0x4];
} HidConsoleSixAxisSensor;

/// HidSharedMemory
typedef struct HidSharedMemory {
    u8 debug_pad[0x400];
    HidTouchScreen touchscreen;
    HidMouse mouse;
    HidKeyboard keyboard;
    u8 digitizer[0x1000];                               ///< [10.0.0+] Digitizer [1.0.0-9.2.0] BasicXpad
    u8 home_button[0x200];
    u8 sleep_button[0x200];
    u8 capture_button[0x200];
    u8 input_detector[0x800];
    u8 unique_pad[0x4000];                              ///< [1.0.0-4.1.0] UniquePad
    HidNpad npad[10];
    u8 gesture[0x800];
    HidConsoleSixAxisSensor console_six_axis_sensor;    ///< [5.0.0+] ConsoleSixAxisSensor
    u8 unk_x3C220[0x3DE0];
} HidSharedMemory;

typedef struct {
    u64 unk_x0;
    u64 unk_x8;
    u64 latest_entry;
    u64 total_entries;
} HidSevenSixAxisSensorStatesHeader;

/// HidSevenSixAxisSensorState
typedef struct {
    u64 timestamp0;
    u64 timestamp1;

    u64 unk_x10;
    float unk_x18[10];
} HidSevenSixAxisSensorState;

/// HidSevenSixAxisSensorStateEntry
typedef struct {
    u64 timestamp;
    u64 unused;
    HidSevenSixAxisSensorState state;
} HidSevenSixAxisSensorStateEntry;

/// HidSevenSixAxisSensorStates
typedef struct {
    HidSevenSixAxisSensorStatesHeader header;
    HidSevenSixAxisSensorStateEntry entries[0x21];
} HidSevenSixAxisSensorStates;

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

static inline u32 hidControllerIDToOfficial(HidControllerID id) {
    if (id < CONTROLLER_HANDHELD) return id;
    if (id == CONTROLLER_HANDHELD) return 0x20;
    return 0x10;//For CONTROLLER_UNKNOWN and invalid values return this.
}

static inline HidControllerID hidControllerIDFromOfficial(u32 id) {
    if (id < 8) return (HidControllerID)id;
    if (id == 0x20) return CONTROLLER_HANDHELD;
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

/// Gets a bitfield of \ref HidNpadStyleTag for the specified controller.
u32 hidGetNpadStyleSet(u32 id);

/// Gets the \ref HidNpadJoyAssignmentMode for the specified controller.
HidNpadJoyAssignmentMode hidGetNpadJoyAssignment(u32 id);

/// Gets the \ref HidNpadControllerColor for the specified controller. colors is the output array, where count is the number of entries. count must be 1 or 2: former for the main colors, latter for reading left/right colors.
Result hidGetNpadControllerColor(u32 id, HidNpadControllerColor *colors, size_t count);

/// Gets the \ref HidDeviceTypeBits for the specified controller.
u32 hidGetNpadDeviceType(u32 id);

/// Gets the \ref HidNpadSystemProperties for the specified controller.
void hidGetNpadSystemProperties(u32 id, HidNpadSystemProperties *out);

/// Gets the \ref HidNpadSystemButtonProperties for the specified controller.
void hidGetNpadSystemButtonProperties(u32 id, HidNpadSystemButtonProperties *out);

/// Gets the \ref HidPowerInfo for the specified controller. info is the output array, where count is the number of entries. count must be 1 or 2: former for the main battery info, latter for reading left/right Joy-Con PowerInfo.
void hidGetNpadPowerInfo(u32 id, HidPowerInfo *info, size_t count);

/// Gets a bitfield of AppletFooterUiAttributes for the specified Npad.
/// Only available on [9.0.0+].
u32 hidGetAppletFooterUiAttributesSet(u32 id);

/// Gets AppletFooterUiTypes for the specified Npad.
/// Only available on [9.0.0+].
u8 hidGetAppletFooterUiTypes(u32 id);

void hidGetNpadStatesFullKey(u32 id, HidNpadFullKeyState *states, size_t count, size_t *total_out);
void hidGetNpadStatesHandheld(u32 id, HidNpadHandheldState *states, size_t count, size_t *total_out);
void hidGetNpadStatesJoyDual(u32 id, HidNpadJoyDualState *states, size_t count, size_t *total_out);
void hidGetNpadStatesJoyLeft(u32 id, HidNpadJoyLeftState *states, size_t count, size_t *total_out);
void hidGetNpadStatesJoyRight(u32 id, HidNpadJoyRightState *states, size_t count, size_t *total_out);
void hidGetNpadStatesSystemExt(u32 id, HidNpadSystemExtState *states, size_t count, size_t *total_out);
void hidGetNpadStatesSystem(u32 id, HidNpadSystemState *states, size_t count, size_t *total_out);

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
Result hidSetSixAxisSensorFusionParameters(u32 SixAxisSensorHandle, float unk0, float unk1);

/// GetSixAxisSensorFusionParameters
Result hidGetSixAxisSensorFusionParameters(u32 SixAxisSensorHandle, float *unk0, float *unk1);

/// ResetSixAxisSensorFusionParameters
Result hidResetSixAxisSensorFusionParameters(u32 SixAxisSensorHandle);

/// Sets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
Result hidSetGyroscopeZeroDriftMode(u32 SixAxisSensorHandle, HidGyroscopeZeroDriftMode mode);

/// Gets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle.
Result hidGetGyroscopeZeroDriftMode(u32 SixAxisSensorHandle, HidGyroscopeZeroDriftMode *mode);

/// Resets the ::HidGyroscopeZeroDriftMode for the specified SixAxisSensorHandle to ::HidGyroscopeZeroDriftMode_Standard.
Result hidResetGyroscopeZeroDriftMode(u32 SixAxisSensorHandle);

/// Sets which controller styles are supported, bitfield of \ref HidNpadStyleTag. This is automatically called with all styles in \ref hidInitialize.
Result hidSetSupportedNpadStyleSet(u32 style_set);

/// Gets which controller styles are supported, bitfield of \ref HidNpadStyleTag.
Result hidGetSupportedNpadStyleSet(u32 *style_set);

/// This is automatically called with CONTROLLER_PLAYER_{1-8} and CONTROLLER_HANDHELD in \ref hidInitialize.
/// count must be <=10. Each entry in buf must be CONTROLLER_PLAYER_{1-8} or CONTROLLER_HANDHELD.
Result hidSetSupportedNpadIdType(HidControllerID *buf, size_t count);

/// Gets an event with the specified autoclear for the input controller.
/// The user *must* close the event when finished with it / before the app exits.
/// This is signaled when the \ref hidGetControllerType output is updated for the controller.
Result hidAcquireNpadStyleSetUpdateEventHandle(HidControllerID id, Event* out_event, bool autoclear);

/// Sets the hold-type, see \ref HidJoyHoldType.
Result hidSetNpadJoyHoldType(HidJoyHoldType type);

/// Gets the hold-type, see \ref HidJoyHoldType.
Result hidGetNpadJoyHoldType(HidJoyHoldType *type);

/// Use this if you want to use a single joy-con as a dedicated CONTROLLER_PLAYER_*.
/// When used, both joy-cons in a pair should be used with this (CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2 for example).
/// id must be CONTROLLER_PLAYER_*.
Result hidSetNpadJoyAssignmentModeSingleByDefault(HidControllerID id);

/// Use this if you want to use a pair of joy-cons as a single CONTROLLER_PLAYER_*. Only necessary if you want to use this mode in your application after \ref hidSetNpadJoyAssignmentModeSingleByDefault was used with this pair of joy-cons.
/// Used automatically during app startup/exit for all controllers.
/// When used, both joy-cons in a pair should be used with this (CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2 for example).
/// id must be CONTROLLER_PLAYER_*.
Result hidSetNpadJoyAssignmentModeDual(HidControllerID id);

/// Merge two single joy-cons into a dual-mode controller. Use this after \ref hidSetNpadJoyAssignmentModeDual, when \ref hidSetNpadJoyAssignmentModeSingleByDefault was previously used (this includes using this manually at application exit).
/// To be successful, id0/id1 must correspond to controller types TYPE_JOYCON_LEFT/TYPE_JOYCON_RIGHT, or TYPE_JOYCON_RIGHT/TYPE_JOYCON_LEFT.
/// If successful, the id of the resulting dual controller is set to id0.
Result hidMergeSingleJoyAsDualJoy(HidControllerID id0, HidControllerID id1);

Result hidInitializeVibrationDevices(u32 *VibrationDeviceHandles, s32 total_handles, HidControllerID id, HidNpadStyleTag style);

/// Gets HidVibrationDeviceInfo for the specified VibrationDeviceHandle.
Result hidGetVibrationDeviceInfo(const u32 *VibrationDeviceHandle, HidVibrationDeviceInfo *VibrationDeviceInfo);

/// Send the VibrationValue to the specified VibrationDeviceHandle.
Result hidSendVibrationValue(const u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue);

/// Gets the current HidVibrationValue for the specified VibrationDeviceHandle.
Result hidGetActualVibrationValue(const u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue);

/// Sets whether vibration is allowed, this also affects the config displayed by System Settings.
Result hidPermitVibration(bool flag);

/// Gets whether vibration is allowed.
Result hidIsVibrationPermitted(bool *flag);

/// Send VibrationValues[index] to VibrationDeviceHandles[index], where count is the number of entries in the VibrationDeviceHandles/VibrationValues arrays.
Result hidSendVibrationValues(const u32 *VibrationDeviceHandles, HidVibrationValue *VibrationValues, s32 count);

/// Gets whether vibration is available with the specified device. Only available on [7.0.0+].
Result hidIsVibrationDeviceMounted(const u32 *VibrationDeviceHandle, bool *flag);

/// Gets SixAxisSensorHandles. total_handles==2 can only be used with ::HidNpadStyleTag_NpadJoyDual.
Result hidGetSixAxisSensorHandles(u32 *SixAxisSensorHandles, s32 total_handles, HidControllerID id, HidNpadStyleTag style);

/// Starts the SixAxisSensor for the specified handle.
Result hidStartSixAxisSensor(u32 SixAxisSensorHandle);

/// Stops the SixAxisSensor for the specified handle.
Result hidStopSixAxisSensor(u32 SixAxisSensorHandle);

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
Result hidGetNpadInterfaceType(HidControllerID id, u8 *out);

