/**
 * @file error.h
 * @brief Wrapper for using the error LibraryApplet.
 * @author StuntHacks
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"

/// Error configuration struct.
typedef struct {
    u8 args[0x1018];
} ErrorConfig;

/**
 * @brief Creates an ErrorConfg struct.
 * @param c ErrorConfg struct.
 * @note Sets the following fields: majorCode = 2000, minorCode = 0, customText = false, shortDescription = "", detailedDescription = "".
 */
void errorCreate(ErrorConfig* c);

/**
 * @brief Closes an ErrorConfg struct.
 * @param c ErrorConfg struct.
 */
void errorClose(ErrorConfig* c);

/**
 * @brief Launches with the specified config.
 * @param c ErrorConfig struct
 */
void errorShow(ErrorConfig* c);

/**
 * @brief Sets the first part of the error code.
 * @param c    ErrorConfig struct
 * @param code Code.
 */
void errorConfigSetMajorCode(ErrorConfig* c, u32 code);

/**
 * @brief Sets the second part of the error code.
 * @param c    ErrorConfig struct
 * @param code Code.
 */
void errorConfigSetMinorCode(ErrorConfig* c, u32 code);

/**
 * @brief Sets whether to use a custom error message.
 * @param c          ErrorConfig struct
 * @param customText Whether to use a custom message.
 */
void errorConfigSetCustomText(ErrorConfig* c, bool customText);

/**
 * @brief Sets the short description.
 * @param c   ErrorConfig struct
 * @param str Description.
 */
void errorConfigSetShortDescription(ErrorConfig* c, const char* str);

/**
 * @brief Sets the detailed description.
 * @param c   ErrorConfig struct
 * @param str Description.
 *
 * This gets displayed when the user clicks on "Details".
 */
void errorConfigSetDetailedDescription(ErrorConfig* c, const char* str);
