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
#include "../services/sm.h"

// Begin enums and output structs

typedef enum
{
    MOUSE_LEFT    = BIT(0),
    MOUSE_RIGHT   = BIT(1),
    MOUSE_MIDDLE  = BIT(2),
    MOUSE_FORWARD = BIT(3),
    MOUSE_BACK    = BIT(4),
} HidMouseButton;

typedef enum
{
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

typedef enum
{
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

/// HID controller type
typedef enum
{
    TYPE_PROCONTROLLER = BIT(0),
    TYPE_HANDHELD      = BIT(1),
    TYPE_JOYCON_PAIR   = BIT(2),
    TYPE_JOYCON_LEFT   = BIT(3),
    TYPE_JOYCON_RIGHT  = BIT(4),
} HidControllerType;

typedef enum
{
    LAYOUT_PROCONTROLLER   = 0, ///< Pro Controller or Hid gamepad.
    LAYOUT_HANDHELD        = 1, ///< Two Joy-Con docked to rails.
    LAYOUT_SINGLE          = 2, ///< Single Joy-Con or pair of Joy-Con, only available in dual-mode with no orientation adjustment.
    LAYOUT_LEFT            = 3, ///< Only single-mode raw left Joy-Con state, no orientation adjustment.
    LAYOUT_RIGHT           = 4, ///< Only single-mode raw right Joy-Con state, no orientation adjustment.
    LAYOUT_DEFAULT_DIGITAL = 5, ///< Same as next, but sticks have 8-direction values only.
    LAYOUT_DEFAULT         = 6, ///< Safe default. Single-mode and ::HidJoyHoldType_Horizontal: Joy-Con have buttons/sticks rotated for orientation, where physical Z(L/R) are unavailable and S(L/R) are mapped to L/R (with physical L/R unavailable).
} HidControllerLayoutType;

typedef enum
{
    COLORS_NONEXISTENT = BIT(1),
} HidControllerColorDescription;

typedef enum
{
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

typedef enum
{
    JOYSTICK_LEFT  = 0,
    JOYSTICK_RIGHT = 1,

    JOYSTICK_NUM_STICKS = 2,
} HidControllerJoystick;

typedef enum
{
    CONTROLLER_STATE_CONNECTED = BIT(0),
    CONTROLLER_STATE_WIRED     = BIT(1),
} HidControllerConnectionState;

typedef enum
{
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
    CONTROLLER_P1_AUTO = 10, ///< Not an actual HID-sysmodule ID. Only for hidKeys*()/hidJoystickRead()/hidSixAxisSensorValuesRead()/hidGetControllerType()/hidGetControllerColors()/hidIsControllerConnected(). Automatically uses CONTROLLER_PLAYER_1 when connected, otherwise uses CONTROLLER_HANDHELD.
} HidControllerID;

typedef enum
{
    HidJoyHoldType_Default    = 0, ///< Default / Joy-Con held vertically.
    HidJoyHoldType_Horizontal = 1, ///< Joy-Con held horizontally with HID state orientation adjustment, see \ref HidControllerLayoutType.
} HidJoyHoldType;

typedef struct touchPosition
{
    u32 id;
    u32 px;
    u32 py;
    u32 dx;
    u32 dy;
    u32 angle;
} touchPosition;

typedef struct JoystickPosition
{
    s32 dx;
    s32 dy;
} JoystickPosition;

typedef struct MousePosition
{
    s32 x;
    s32 y;
    s32 velocityX;
    s32 velocityY;
    s32 scrollVelocityX;
    s32 scrollVelocityY;
} MousePosition;

typedef struct HidVector
{
    float x;
    float y;
    float z;
} HidVector;

typedef struct SixAxisSensorValues
{
    HidVector accelerometer;
    HidVector gyroscope;
    HidVector unk;
    HidVector orientation[3];
} SixAxisSensorValues;

#define JOYSTICK_MAX (0x8000)
#define JOYSTICK_MIN (-0x8000)

// End enums and output structs

// Begin HidTouchScreen

typedef struct HidTouchScreenHeader
{
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
    u64 timestamp;
} HidTouchScreenHeader;

typedef struct HidTouchScreenEntryHeader
{
    u64 timestamp;
    u64 numTouches;
} HidTouchScreenEntryHeader;

typedef struct HidTouchScreenEntryTouch
{
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

typedef struct HidTouchScreenEntry
{
    HidTouchScreenEntryHeader header;
    HidTouchScreenEntryTouch touches[16];
    u64 unk;
} HidTouchScreenEntry;

typedef struct HidTouchScreen
{
    HidTouchScreenHeader header;
    HidTouchScreenEntry entries[17];
    u8 padding[0x3c0];
} HidTouchScreen;

// End HidTouchScreen

// Begin HidMouse

typedef struct HidMouseHeader
{
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
} HidMouseHeader;

typedef struct HidMouseEntry
{
    u64 timestamp;
    u64 timestamp_2;
    MousePosition position;
    u64 buttons;
} HidMouseEntry;

typedef struct HidMouse
{
    HidMouseHeader header;
    HidMouseEntry entries[17];
    u8 padding[0xB0];
} HidMouse;

// End HidMouse

// Begin HidKeyboard

typedef struct HidKeyboardHeader
{
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
} HidKeyboardHeader;

typedef struct HidKeyboardEntry
{
    u64 timestamp;
    u64 timestamp_2;
    u64 modifier;
    u32 keys[8];
} HidKeyboardEntry;

typedef struct HidKeyboard
{
    HidKeyboardHeader header;
    HidKeyboardEntry entries[17];
    u8 padding[0x28];
} HidKeyboard;

// End HidKeyboard

// Begin HidController

typedef struct HidControllerMAC
{
    u64 timestamp;
    u8 mac[0x8];
    u64 unk;
    u64 timestamp_2;
} HidControllerMAC;

typedef struct HidControllerHeader
{
    u32 type;
    u32 isHalf;
    u32 singleColorsDescriptor;
    u32 singleColorBody;
    u32 singleColorButtons;
    u32 splitColorsDescriptor;
    u32 leftColorBody;
    u32 leftColorButtons;
    u32 rightColorBody;
    u32 rightColorButtons;
} HidControllerHeader;

/// Info struct extracted from HidControllerHeader.
/// Color fields are zero when not set. This can happen even when the *Set fields are set to true.
typedef struct HidControllerColors
{
    bool singleSet;         ///< Set to true when the below fields are valid.
    u32 singleColorBody;    ///< RGBA Single Body Color
    u32 singleColorButtons; ///< RGBA Single Buttons Color

    bool splitSet;          ///< Set to true when the below fields are valid.
    u32 leftColorBody;      ///< RGBA Left Body Color
    u32 leftColorButtons;   ///< RGBA Left Buttons Color
    u32 rightColorBody;     ///< RGBA Right Body Color
    u32 rightColorButtons;  ///< RGBA Right Buttons Color
} HidControllerColors;

typedef struct HidControllerLayoutHeader
{
    u64 timestampTicks;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
} HidControllerLayoutHeader;

typedef struct HidControllerInputEntry
{
    u64 timestamp;
    u64 timestamp_2;
    u64 buttons;
    JoystickPosition joysticks[JOYSTICK_NUM_STICKS];
    u64 connectionState;
} HidControllerInputEntry;

typedef struct HidControllerLayout
{
    HidControllerLayoutHeader header;
    HidControllerInputEntry entries[17];
} HidControllerLayout;

typedef struct HidControllerSixAxisHeader
{
    u64 timestamp;
    u64 numEntries;
    u64 latestEntry;
    u64 maxEntryIndex;
} HidControllerSixAxisHeader;

typedef struct HidControllerSixAxisEntry
{
    u64 timestamp;
    u64 unk_1;
    u64 timestamp_2;
    SixAxisSensorValues values;
    u64 unk_3;
} HidControllerSixAxisEntry;

typedef struct HidControllerSixAxisLayout
{
    HidControllerSixAxisHeader header;
    HidControllerSixAxisEntry entries[17];
} HidControllerSixAxisLayout;

/// Controller flags.
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

    u32 unintendedHomeButtonInputProtectionDisabled : 1;
} HidFlags;

typedef struct {
    u32 deviceType;
    u32 pad;
    HidFlags flags;
    u32 batteryCharge[3];
    u8 unk_1[0x20];
    HidControllerMAC macLeft;
    HidControllerMAC macRight;
} HidControllerMisc;

typedef struct {
    bool powerConnected;
    bool isCharging;
    u32 batteryCharge;    ///< Battery charge, always 0-4.
} HidPowerInfo;

typedef struct HidController
{
    HidControllerHeader header;
    HidControllerLayout layouts[7];
    HidControllerSixAxisLayout sixaxis[6];
    HidControllerMisc misc;
    u8 unk_2[0xDF8];
} HidController;

// End HidController

typedef struct HidSharedMemory
{
    u8 header[0x400];
    HidTouchScreen touchscreen;
    HidMouse mouse;
    HidKeyboard keyboard;
    u8 unkSection1[0x400];
    u8 unkSection2[0x400];
    u8 unkSection3[0x400];
    u8 unkSection4[0x400];
    u8 unkSection5[0x200];
    u8 unkSection6[0x200];
    u8 unkSection7[0x200];
    u8 unkSection8[0x800];
    u8 controllerSerials[0x4000];
    HidController controllers[10];
    u8 unkSection9[0x4600];
} HidSharedMemory;

typedef struct HidVibrationDeviceInfo
{
    u32 unk_x0;
    u32 unk_x4; ///< 0x1 for left-joycon, 0x2 for right-joycon.
} HidVibrationDeviceInfo;

typedef struct HidVibrationValue
{
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

/// Initializes hid, called automatically during app startup.
Result hidInitialize(void);
void hidExit(void);
void hidReset(void);

Service* hidGetServiceSession(void);
void* hidGetSharedmemAddr(void);

void hidSetControllerLayout(HidControllerID id, HidControllerLayoutType layoutType);
HidControllerLayoutType hidGetControllerLayout(HidControllerID id);
/// Gets the \ref HidControllerType for the specified controller.
HidControllerType hidGetControllerType(HidControllerID id);
void hidGetControllerColors(HidControllerID id, HidControllerColors *colors);
bool hidIsControllerConnected(HidControllerID id);

/// Gets the DeviceType for the specified controller.
u32 hidGetControllerDeviceType(HidControllerID id);

/// Gets the flags for the specified controller.
void hidGetControllerFlags(HidControllerID id, HidFlags *flags);

/// Gets the \ref HidPowerInfo for the specified controller. info is the output array, where total_info is the number of entries. total_info must be 1 or 2: former for the main battery info, latter for reading left/right Joy-Con PowerInfo.
void hidGetControllerPowerInfo(HidControllerID id, HidPowerInfo *info, size_t total_info);

void hidScanInput(void);

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

/// This is automatically called with CONTROLLER_PLAYER_{1-8} and CONTROLLER_HANDHELD in \ref hidInitialize.
/// count must be <=10. Each entry in buf must be CONTROLLER_PLAYER_{1-8} or CONTROLLER_HANDHELD.
Result hidSetSupportedNpadIdType(HidControllerID *buf, size_t count);

/// Sets which controller types are supported. This is automatically called with all types in \ref hidInitialize.
Result hidSetSupportedNpadStyleSet(HidControllerType type);

/// Gets an event with the specified autoclear for the input controller.
/// The user *must* close the event when finished with it / before the app exits.
/// This is signaled when the \ref hidGetControllerType output is updated for the controller.
Result hidAcquireNpadStyleSetUpdateEventHandle(HidControllerID id, Event* event, bool autoclear);

/// Sets the hold-type, see \ref HidJoyHoldType.
Result hidSetNpadJoyHoldType(HidJoyHoldType type);

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

Result hidInitializeVibrationDevices(u32 *VibrationDeviceHandles, size_t total_handles, HidControllerID id, HidControllerType type);

/// Gets HidVibrationDeviceInfo for the specified VibrationDeviceHandle.
Result hidGetVibrationDeviceInfo(u32 *VibrationDeviceHandle, HidVibrationDeviceInfo *VibrationDeviceInfo);

/// Send the VibrationValue to the specified VibrationDeviceHandle.
Result hidSendVibrationValue(u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue);

/// Gets the current HidVibrationValue for the specified VibrationDeviceHandle.
Result hidGetActualVibrationValue(u32 *VibrationDeviceHandle, HidVibrationValue *VibrationValue);

/// Sets whether vibration is allowed, this also affects the config displayed by System Settings.
Result hidPermitVibration(bool flag);

/// Gets whether vibration is allowed.
Result hidIsVibrationPermitted(bool *flag);

/// Send VibrationValues[index] to VibrationDeviceHandles[index], where count is the number of entries in the VibrationDeviceHandles/VibrationValues arrays.
Result hidSendVibrationValues(u32 *VibrationDeviceHandles, HidVibrationValue *VibrationValues, size_t count);

/// Gets SixAxisSensorHandles. total_handles==2 can only be used with TYPE_JOYCON_PAIR.
Result hidGetSixAxisSensorHandles(u32 *SixAxisSensorHandles, size_t total_handles, HidControllerID id, HidControllerType type);

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

