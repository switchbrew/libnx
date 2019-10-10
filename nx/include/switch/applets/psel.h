/**
 * @file psel.h
 * @brief Wrapper for using playerSelect (user selection applet).
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/applet.h"
#include "../services/acc.h"

/// playerSelect UI modes.
typedef enum {
    PselUiMode_SelectUser                           = 0,  ///< Simple user selection (new users cannot be created).
    PselUiMode_UserCreation                         = 1,  ///< Only user creation (the user is later returned).
    PselUiMode_EnsureNsaAvailable                   = 2,  ///< EnsureNsaAvailable
    PselUiMode_IconEditor                           = 3,  ///< IconEditor
    PselUiMode_NicknameEditor                       = 4,  ///< NicknameEditor
    PselUiMode_ForStarter                           = 5,  ///< Mode "starter" uses to register the console's first user on the initial setup
    PselUiMode_NetworkServiceAccountRegistration    = 8,  ///< NetworkServiceAccountRegistration
    PselUiMode_NintendoAccountNnidLinker            = 9,  ///< NintendoAccountNnidLinker
    PselUiMode_LicenseRequirementsForNetworkService = 10, ///< LicenseRequirementsForNetworkService
    PselUiMode_NaLoginTest                          = 12, ///< NaLoginTest
} PselUiMode;

/// UI settings for playerSelect.
typedef struct {
    u32 mode;                                         ///< UI mode, see \ref PselUiMode.
    u32 dialogType;                                   ///< Dialog type
    AccountUid invalidUserList[ACC_USER_LIST_SIZE];   ///< List of \ref AccountUid user IDs which will be disabled.
    u8 unk_x88[0x8];                                  ///< Unknown.
    u8 networkServiceRequired;                        ///< Whether the user needs to be linked to a Nintendo account.
    u8 unk_x91[0x2];                                  ///< Unknown.
    u8 allowUserCreation;                             ///< (With ::PselUiMode_SelectUser) enables the option to create a new user.
    u8 skipEnabled;                                   ///< Enables the option to skip user selection (a new button is shown)
    u8 unk_x95[0xb];                                   ///< Unknown.
} PselUiSettings;

/// Result data sent after execution.
typedef struct {
    u32 result;                                       ///< Result code.
    AccountUid userId;                                ///< Selected \ref AccountUid.
} PselResult;

/**
 * @brief Creates a new UI config for playerSelect applet with the specified mode.
 * @param ui PseluiSettings struct.
 * @param mode playerSelect UI mode.
 */
Result pselUiCreate(PselUiSettings *ui, PselUiMode mode);

/**
 * @brief Adds a user to the invalid user list (these users will be blacklisted in the applet)
 * @param ui PselUiSettings struct.
 * @param user_id user ID.
 */
void pselUiAddInvalidUser(PselUiSettings *ui, AccountUid *user_id);

/**
 * @brief Sets whether users can be created in the applet
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 * @note Only used for ::PselUiMode_SelectUser
 */
void pselUiSetAllowUserCreation(PselUiSettings *ui, bool flag);

/**
 * @brief Sets whether users need to be linked to a Nintendo account.
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
void pselUiSetNetworkServiceRequired(PselUiSettings *ui, bool flag);

/**
 * @brief Sets whether selection can be skipped (with a new button)
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
void pselUiSetSkipEnabled(PselUiSettings *ui, bool flag);

/**
 * @brief Shows the applet with the specified UI settings.
 * @param ui PselUiSettings struct.
 * @param out_uid Selected user ID.
 * @note If user skips (see \ref pselUiSetSkipEnabled) this will return successfully but the output ID will be 0.
 */
Result pselUiShow(PselUiSettings *ui, AccountUid *out_user);