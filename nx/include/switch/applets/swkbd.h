/**
 * @file swkbd.h
 * @brief Wrapper for using the swkbd LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

typedef enum {
    SwkbdTextCheckResult_OK      = 0,  ///< Success, valid string.
    SwkbdTextCheckResult_Bad     = 1,  ///< Failure, invalid string. Error message is displayed in a message-box, pressing OK will return to swkbd again.
    SwkbdTextCheckResult_Prompt  = 2,  ///< Failure, invalid string. Error message is displayed in a message-box, pressing Cancel will return to swkbd again, while pressing OK will continue as if the text was valid.
    SwkbdTextCheckResult_Silent  = 3,  ///< Failure, invalid string. With value 3 and above, swkbd will silently not accept the string, without displaying any error.
} SwkbdTextCheckResult;

typedef SwkbdTextCheckResult (*SwkbdTextCheckCb)(char* tmp_string, size_t tmp_string_size); /// TextCheck callback set by \ref swkbdConfigSetTextCheckCallback, for validating the input string when the swkbd ok-button is pressed. This buffer contains an UTF-8 string. This callback should validate the input string, then return a \ref SwkbdTextCheckResult indicating success/failure. On failure, this function must write an error message to the tmp_string buffer, which will then be displayed by swkbd.

/// Base swkbd arg struct.
typedef struct {
    u32 unk_x0;
    u16 okButtonText[18/2];
    u16 leftButtonText;
    u16 rightButtonText;
    u16 dicFlag;                   ///< Enables dictionary usage when non-zero (including the system dictionary).
    u32 keySetDisableBitmask;
    u32 initialCursorPos;          ///< Initial cursor position in the string: 0 = start, 1 = end.
    u16 headerText[130/2];
    u16 subText[258/2];
    u16 guideText[514/2];
    u16 pad_x3aa;
    u32 stringLenMax;              ///< When non-zero, specifies the max string length. When the input is too long, swkbd will stop accepting more input until text is deleted via the B button (Backspace). When this is <=32, this will also display the string on a line instead of in a text-box, and enables displaying the Header and Sub text.
    u32 stringLenMaxExt;           ///< When non-zero, specifies the max string length. When the input is too long, swkbd will display an icon and disable the ok-button.
    u32 passwordFlag;              ///< Use password: 0 = disable, 1 = enable.
    u32 unk_x3b8;
    u16 returnButtonFlag;          ///< Controls whether the Return button is enabled, for newlines input. 0 = disabled, non-zero = enabled.
    u8  blurBackground;            ///< When enabled with value 1, the background is blurred.
    u8  pad_x3bf;
    u32 initialStringOffset;
    u32 initialStringSize;
    u32 userDicOffset;
    s32 userDicEntries;
    u8 textCheckFlag;
    u8 pad_x3d1[7];
    SwkbdTextCheckCb textCheckCb;  ///< This really doesn't belong in a struct sent to another process, but official sw does this.
} SwkbdArgV0;

/// Arg struct for version 0x30007+.
typedef struct {
    SwkbdArgV0 arg;
    u32 unk_x3e0[8];
} SwkbdArgV7;

typedef struct {
    SwkbdArgV7 arg;

    u8* workbuf;
    size_t workbuf_size;
    s32 max_dictwords;

    u32 version;
} SwkbdConfig;

/// User dictionary word.
typedef struct {
    u8 unk_x0[0x64];
} SwkbdDictWord;

/**
 * @brief Creates a SwkbdConfig struct.
 * @param c SwkbdConfig struct.
 * @param max_dictwords Max \ref SwkbdDictWord entries, 0 for none.
 */
Result swkbdCreate(SwkbdConfig* c, s32 max_dictwords);

/**
 * @brief Closes a SwkbdConfig struct.
 * @param c SwkbdConfig struct.
 */
void swkbdClose(SwkbdConfig* c);

/**
 * @brief Clears the args in the SwkbdConfig struct and initializes it with the Default Preset.
 * @note Do not use this before \ref swkbdCreate.
 * @param c SwkbdConfig struct.
 */
void swkbdConfigMakePresetDefault(SwkbdConfig* c);

/**
 * @brief Clears the args in the SwkbdConfig struct and initializes it with the Password Preset.
 * @note Do not use this before \ref swkbdCreate.
 * @param c SwkbdConfig struct.
 */
void swkbdConfigMakePresetPassword(SwkbdConfig* c);

/**
 * @brief Clears the args in the SwkbdConfig struct and initializes it with the UserName Preset.
 * @note Do not use this before \ref swkbdCreate.
 * @param c SwkbdConfig struct.
 */
void swkbdConfigMakePresetUserName(SwkbdConfig* c);

/**
 * @brief Clears the args in the SwkbdConfig struct and initializes it with the DownloadCode Preset.
 * @note Do not use this before \ref swkbdCreate.
 * @param c SwkbdConfig struct.
 */
void swkbdConfigMakePresetDownloadCode(SwkbdConfig* c);

/**
 * @brief Sets the Ok button text. The default is "".
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetOkButtonText(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the LeftOptionalSymbolKey. The default is "".
 * @note The swkbd applet currently doesn't display this, unknown why.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetLeftOptionalSymbolKey(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the RightOptionalSymbolKey. The default is "".
 * @note The swkbd applet currently doesn't display this, unknown why.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetRightOptionalSymbolKey(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the Header text. The default is "".
 * @note See \ref SwkbdArgV0 stringLenMax.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetHeaderText(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the Sub text. The default is "".
 * @note See \ref SwkbdArgV0 stringLenMax.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetSubText(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the Guide text. The default is "".
 * @note The swkbd applet only displays this when the current displayed cursor position is 0.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetGuideText(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the Initial text. The default is "".
 * @note The swkbd applet currently doesn't display this, unknown why.
 * @param c SwkbdConfig struct.
 * @param str UTF-8 input string.
 */
void swkbdConfigSetInitialText(SwkbdConfig* c, const char* str);

/**
 * @brief Sets the user dictionary.
 * @param c SwkbdConfig struct.
 * @param input Input data.
 * @param entries Total entries in the buffer.
 */
void swkbdConfigSetDictionary(SwkbdConfig* c, const SwkbdDictWord *input, s32 entries);

/**
 * @brief Sets the TextCheck callback.
 * @param c SwkbdConfig struct.
 * @param cb \ref SwkbdTextCheckCb callback.
 */
void swkbdConfigSetTextCheckCallback(SwkbdConfig* c, SwkbdTextCheckCb cb);

/**
 * @brief Launch swkbd with the specified config. This will return once swkbd is finished running.
 * @note The string buffer is also used for the buffer passed to the \ref SwkbdTextCheckCb, when it's set. Hence, in that case this buffer should be large enough to handle TextCheck string input/output. The size passed to the callback is the same size passed here, -1.
 * @param c SwkbdConfig struct.
 * @param out_string UTF-8 Output string buffer.
 * @param out_string_size UTF-8 Output string buffer size, including NUL-terminator.
 */
Result swkbdShow(SwkbdConfig* c, char* out_string, size_t out_string_size);

