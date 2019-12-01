/**
 * @file psel.h
 * @brief Wrapper for using playerSelect (user selection applet).
 * @author XorTroll, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/acc.h"

/// playerSelect UI modes.
typedef enum {
    PselUiMode_UserSelector                                            = 0,  ///< Simple user selection (new users cannot be created).
    PselUiMode_UserCreator                                             = 1,  ///< User creation.
    PselUiMode_EnsureNetworkServiceAccountAvailable                    = 2,  ///< EnsureNetworkServiceAccountAvailable
    PselUiMode_UserIconEditor                                          = 3,  ///< UserIconEditor
    PselUiMode_UserNicknameEditor                                      = 4,  ///< UserNicknameEditor
    PselUiMode_UserCreatorForStarter                                   = 5,  ///< Mode "starter" uses to register the console's first user on the initial setup.
// TODO: Add mode 6.
    PselUiMode_IntroduceExternalNetworkServiceAccount                  = 7,  ///< IntroduceExternalNetworkServiceAccount
    PselUiMode_IntroduceExternalNetworkServiceAccountForRegistration   = 8,  ///< [6.0.0+] IntroduceExternalNetworkServiceAccountForRegistration
    PselUiMode_NintendoAccountNnidLinker                               = 9,  ///< [6.0.0+] NintendoAccountNnidLinker
    PselUiMode_LicenseRequirementsForNetworkService                    = 10, ///< [6.0.0+] LicenseRequirementsForNetworkService
    PselUiMode_LicenseRequirementsForNetworkServiceWithUserContextImpl = 11, ///< [7.0.0+] LicenseRequirementsForNetworkServiceWithUserContextImpl
    PselUiMode_UserCreatorForImmediateNaLoginTest                      = 12, ///< [7.0.0+] UserCreatorForImmediateNaLoginTest
} PselUiMode;

/// Base UI settings for playerSelect.
typedef struct {
    u32 mode;                                          ///< \ref PselUiMode
    u32 dialog_type;                                   ///< Dialog type.
    AccountUid user_list[ACC_USER_LIST_SIZE];          ///< List of \ref AccountUid.
    u8 unk_x88[0x8];                                   ///< Unknown.
    u8 network_service_required;                       ///< Whether the user needs to be linked to a Nintendo account.
    u8 unk_x91[0x2];                                   ///< Unknown.
    u8 allow_user_creation;                            ///< (With ::PselUiMode_SelectUser) enables the option to create a new user.
    u8 skip_enabled;                                   ///< Enables the option to skip user selection (a new button is shown)
    u8 unk_x95[0x3];                                   ///< Unknown.
} PselUiSettingsV1;

/// UI settings for versions starting with 0x10000.
typedef struct {
    PselUiSettingsV1 settings;                         ///< \ref PselUiSettingsV1
    u8 unk_x98[0x8];                                   ///< Unknown.
} PselUiSettings;

/// Return data sent after execution.
typedef struct {
    Result res;                                       ///< Result.
    AccountUid user_id;                               ///< Selected \ref AccountUid.
} PselUiReturnArg;

/**
 * @brief Creates a new UI config for the playerSelect applet with the specified mode.
 * @param ui PseluiSettings struct.
 * @param mode playerSelect UI mode.
 */
Result pselUiCreate(PselUiSettings *ui, PselUiMode mode);

/**
 * @brief Adds an user to the user list of the applet.
 * @param ui PselUiSettings struct.
 * @param[in] user_id user ID.
 * @note The users will be treated as invalid users for user selection mode, and as the input user for other modes.
 */
void pselUiAddUser(PselUiSettings *ui, AccountUid user_id);

/**
 * @brief Sets whether users can be created in the applet.
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 * @note Only used for ::PselUiMode_SelectUser
 */
NX_CONSTEXPR void pselUiSetAllowUserCreation(PselUiSettings *ui, bool flag) {
    if(ui->settings.mode == PselUiMode_UserSelector) {
        ui->settings.allow_user_creation = flag;
    }
}

/**
 * @brief Sets whether users need to be linked to a Nintendo account.
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
NX_CONSTEXPR void pselUiSetNetworkServiceRequired(PselUiSettings *ui, bool flag) {
    ui->settings.network_service_required = flag;
}

/**
 * @brief Sets whether selection can be skipped (with a new button).
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
NX_CONSTEXPR void pselUiSetSkipEnabled(PselUiSettings *ui, bool flag) {
    ui->settings.skip_enabled = flag;
}

/**
 * @brief Shows the applet with the specified UI settings.
 * @param ui PselUiSettings struct.
 * @param out_user Selected user ID.
 * @note If user skips (see \ref pselUiSetSkipEnabled) this will return successfully but the output ID will be 0.
 */
Result pselUiShow(PselUiSettings *ui, AccountUid *out_user);

/**
 * @brief Shows the applet to select a user.
 * @param[out] out_user Returned selected user ID.
 */
Result pselShowUserSelector(AccountUid *out_user);

/**
 * @brief Shows the applet to create a user.
 */
Result pselShowUserCreator(void);

/**
 * @brief Shows the applet to change a user's icon.
 * @param[in] user Input user ID.
 */
Result pselShowUserIconEditor(AccountUid user);

/**
 * @brief Shows the applet to change a user's nickname.
 * @param[in] user Input user ID.
 */
Result pselShowUserNicknameEditor(AccountUid user);

/**
 * @brief Shows the applet to create a user. Used by the starter applet during system setup.
 */
Result pselShowUserCreatorForStarter(void);

/**
 * @brief Shows the applet for Nintendo Account Nnid linking.
 * @note Only available on [6.0.0+].
 * @param[in] user Input user ID.
 */
Result pselShowNintendoAccountNnidLinker(AccountUid user);
