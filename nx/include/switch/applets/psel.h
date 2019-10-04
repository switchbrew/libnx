/**
 * @file psel.h
 * @brief Wrapper for using playerSelect applet (user selection applet).
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"
#include "../services/set.h"

// playerSelect applet modes. There are more of them related to network accounts.
typedef enum {
    PselMode_Normal            = 0,  ///< Simple user selection (new users cannot be created).
    PselMode_UserCreation      = 1,  ///< Only user creation (the user is later returned).
    PselMode_ForStarter        = 5,  ///< Mode "starter" uses to register the console's first user on the initial setup
} PselMode;

// Config data for playerSelect applet.
typedef struct {
    u32 mode;                        ///< Mode, see \ref PselMode.
    u8 unk1[0x8f];                   ///< Unknown
    u8 userCreationFlag;             ///< If set, a new user can be created and chosen (for ::PselMode_Normal).
    u8 omitOptionFlag;               ///< If set, an "Omit" button is shown.
    u8 unk2[11];                      ///< Unknown
} PselUserSelectionConfig;

// Result data returned after execution.
typedef struct {
    u32 result;                      ///< Result (0 = Success, 2 = Failure)
    u128 userId;                     ///< UUID of selected user
} PselResult;

// Config holder structure.
typedef struct {
    PselUserSelectionConfig config;  ///< User selection config
} PselConfig;

/**
 * @brief Creates a new config for playerSelect applet with the specified mode.
 * @param c PselConfig struct.
 * @param mode playerSelect applet mode.
 */
Result pselConfigCreate(PselConfig *c, PselMode mode);

/**
 * @brief Sets the UserCreation flag.
 * @param c PselConfig struct.
 * @param flag Flag value.
 */
static inline void pselConfigSetUserCreationFlag(PselConfig *c, bool flag) {
    c->config.userCreationFlag = flag;
}

/**
 * @brief Sets the OmitOption flag.
 * @param c PselConfig struct.
 * @param flag Flag value.
 */
static inline void pselConfigSetOmitOptionFlag(PselConfig *c, bool flag) {
    c->config.omitOptionFlag = flag;
}

/**
 * @brief Shows the applet with the specified config.
 * @param c PselConfig struct.
 * @param out_uid Output user ID.
 */
Result pselConfigShow(PselConfig *c, u128 *out_uid);
