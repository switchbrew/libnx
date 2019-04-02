/**
 * @file pctlauth.h
 * @brief Wrapper for using the Parental Controls authentication LibraryApplet. This applet is used by qlaunch.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Type values for PctlAuthArg::type.
typedef enum {
    PctlAuthType_Show             = 0,  ///< ShowParentalAuthentication
    PctlAuthType_RegisterPasscode = 1,  ///< RegisterParentalPasscode
    PctlAuthType_ChangePasscode   = 2,  ///< ChangeParentalPasscode
} PctlAuthType;

/// Input arg storage for the applet.
typedef struct {
    u32 unk_x0;         ///< Always set to 0 by the user-process.
    PctlAuthType type;  ///< \ref PctlAuthType
    u8 arg0;            ///< Arg0
    u8 arg1;            ///< Arg1
    u8 arg2;            ///< Arg2
    u8 pad;             ///< Padding
} PctlAuthArg;

/**
 * @brief Launches the applet.
 * @note Should not be used if a PIN is not already registered. See \ref pctlIsRestrictionEnabled.
 * @param flag Input flag. false = temporarily disable Parental Controls. true = validate the input PIN.
 */
Result pctlauthShow(bool flag);

/**
 * @brief Launches the applet. Only available with [4.0.0+].
 * @param arg0 Value for PctlAuthArg.arg0.
 * @param arg1 Value for PctlAuthArg.arg1.
 * @param arg2 Value for PctlAuthArg.arg2.
 */
Result pctlauthShowEx(u8 arg0, u8 arg1, u8 arg2);

/**
 * @brief Just calls: pctlauthShowEx(1, 0, 1). Launches the applet for checking the PIN, used when changing system-settings.
 * @note Should not be used if a PIN is not already registered. See \ref pctlIsRestrictionEnabled.
 */
Result pctlauthShowForConfiguration(void);

/**
 * @brief Launches the applet for registering the Parental Controls PIN.
 */
Result pctlauthRegisterPasscode(void);

/**
 * @brief Launches the applet for changing the Parental Controls PIN.
 * @note Should not be used if a PIN is not already registered. See \ref pctlIsRestrictionEnabled.
 */
Result pctlauthChangePasscode(void);

