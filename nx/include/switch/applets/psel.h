/**
 * @file psel.h
 * @brief Wrapper for using the playerSelect (user selection) LibraryApplet.
 * @author XorTroll, yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/acc.h"

/// playerSelect UI modes.
typedef enum {
    PselUiMode_UserSelector                                            = 0,  ///< UserSelector
    PselUiMode_UserCreator                                             = 1,  ///< UserCreator
    PselUiMode_EnsureNetworkServiceAccountAvailable                    = 2,  ///< EnsureNetworkServiceAccountAvailable
    PselUiMode_UserIconEditor                                          = 3,  ///< UserIconEditor
    PselUiMode_UserNicknameEditor                                      = 4,  ///< UserNicknameEditor
    PselUiMode_UserCreatorForStarter                                   = 5,  ///< UserCreatorForStarter
    PselUiMode_NintendoAccountAuthorizationRequestContext              = 6,  ///< NintendoAccountAuthorizationRequestContext
    PselUiMode_IntroduceExternalNetworkServiceAccount                  = 7,  ///< IntroduceExternalNetworkServiceAccount
    PselUiMode_IntroduceExternalNetworkServiceAccountForRegistration   = 8,  ///< [6.0.0+] IntroduceExternalNetworkServiceAccountForRegistration
    PselUiMode_NintendoAccountNnidLinker                               = 9,  ///< [6.0.0+] NintendoAccountNnidLinker
    PselUiMode_LicenseRequirementsForNetworkService                    = 10, ///< [6.0.0+] LicenseRequirementsForNetworkService
    PselUiMode_LicenseRequirementsForNetworkServiceWithUserContextImpl = 11, ///< [7.0.0+] LicenseRequirementsForNetworkServiceWithUserContextImpl
    PselUiMode_UserCreatorForImmediateNaLoginTest                      = 12, ///< [7.0.0+] UserCreatorForImmediateNaLoginTest
} PselUiMode;

/// UI message text to display with ::PselUiMode_UserSelector. Invalid values are handled as ::PselUserSelectionPurpose_General.
typedef enum {
    PselUserSelectionPurpose_General                                   = 0,  ///< "Select a user."
    PselUserSelectionPurpose_GameCardRegistration                      = 1,  ///< [2.0.0+] "Who will receive the points?"
    PselUserSelectionPurpose_EShopLaunch                               = 2,  ///< [2.0.0+] "Who is using Nintendo eShop?"
    PselUserSelectionPurpose_EShopItemShow                             = 3,  ///< [2.0.0+] "Who is making this purchase?"
    PselUserSelectionPurpose_PicturePost                               = 4,  ///< [2.0.0+] "Who is posting?"
    PselUserSelectionPurpose_NintendoAccountLinkage                    = 5,  ///< [2.0.0+] "Select a user to link to a Nintendo Account."
    PselUserSelectionPurpose_SettingsUpdate                            = 6,  ///< [2.0.0+] "Change settings for which user?"
    PselUserSelectionPurpose_SaveDataDeletion                          = 7,  ///< [2.0.0+] "Format data for which user?"
    PselUserSelectionPurpose_UserMigration                             = 8,  ///< [4.0.0+] "Which user will be transferred to another console?"
    PselUserSelectionPurpose_SaveDataTransfer                          = 9,  ///< [8.0.0+] "Send save data for which user?"
} PselUserSelectionPurpose;

/// NintendoAccountStartupDialogType
typedef enum {
    PselNintendoAccountStartupDialogType_LoginAndCreate                = 0,  ///< LoginAndCreate
    PselNintendoAccountStartupDialogType_Login                         = 1,  ///< Login
    PselNintendoAccountStartupDialogType_Create                        = 2,  ///< Create
} PselNintendoAccountStartupDialogType;

/// Base UI settings for playerSelect.
typedef struct {
    u32 mode;                                          ///< \ref PselUiMode
    u32 pad;                                           ///< Padding.
    AccountUid invalid_uid_list[ACC_USER_LIST_SIZE];   ///< List of \ref AccountUid. TODO: This is only correct for ::PselUiMode_UserSelector, for other modes this is a single uid, followed by mode-specific data (if any).
    u64 application_id;                                ///< ApplicationId with \ref pselShowUserSelectorForLauncher.
    u8 is_network_service_account_required;            ///< PselUserSelectionSettings::is_network_service_account_required.
    u8 is_skip_enabled;                                ///< PselUserSelectionSettings::is_skip_enabled
    u8 unk_x92;                                        ///< Set to value 1 by \ref pselShowUserSelectorForSystem / \ref pselShowUserSelectorForLauncher.
    u8 is_permitted;                                   ///< isPermitted. With ::PselUiMode_UserSelector: enables the option to create a new user. Set to the output from \ref accountIsUserRegistrationRequestPermitted with pselShowUserSelector*. When not set, a dialog will be displayed when the user attempts to create an user.
    u8 show_skip_button;                               ///< PselUserSelectionSettings::show_skip_button
    u8 additional_select;                              ///< PselUserSelectionSettings::additional_select
    u8 unk_x96;                                        ///< [2.0.0+] Set to PselUserSelectionSettingsForSystemService::enable_user_creation_button. \ref pselShowUserSelectorForLauncher / \ref pselShowUserSelector sets this to value 1.
    u8 unk_x97;                                        ///< [6.0.0+] Set to PselUserSelectionSettings::is_unqualified_user_selectable ^ 1.
} PselUiSettingsV1;

/// UI settings for versions starting with 0x10000.
typedef struct {
    PselUiSettingsV1 settings;                         ///< \ref PselUiSettingsV1
    u32 unk_x98;                                       ///< [2.0.0+] Set to PselUserSelectionSettingsForSystemService::purpose.
    u8 unk_x9c[0x4];                                   ///< Unknown.
} PselUiSettings;

/// UserSelectionSettings
typedef struct {
    AccountUid invalid_uid_list[ACC_USER_LIST_SIZE];   ///< invalidUidList.
    u8 is_skip_enabled;                                ///< isSkipEnabled. When set, the first user in invalid_uid_list must not be set, and additional_select must be 0. When enabled \ref accountTrySelectUserWithoutInteraction will be used to select the user, in this case the applet will only be launched if \ref accountTrySelectUserWithoutInteraction doesn't return an user.
    u8 is_network_service_account_required;            ///< isNetworkServiceAccountRequired. Whether the user needs to be linked to a Nintendo account.
    u8 show_skip_button;                               ///< showSkipButton. Enables the option to skip user selection with a button.
    u8 additional_select;                              ///< additionalSelect.
    u8 is_unqualified_user_selectable;                 ///< [6.0.0+] isUnqualifiedUserSelectable
} PselUserSelectionSettings;

/// [2.0.0+] UserSelectionSettingsForSystemService
typedef struct {
    u32 purpose;                                       ///< \ref PselUserSelectionPurpose
    u8 enable_user_creation_button;                    ///< Enables the user-creation button when set. Whether user-creation when pressing the button is actually allowed is controlled by PselUiSettingsV1::is_permitted.
    u8 pad[0x3];                                       ///< Padding.
} PselUserSelectionSettingsForSystemService;

/// Return data sent after execution.
typedef struct {
    Result res;                                        ///< Result.
    AccountUid user_id;                                ///< Selected \ref AccountUid.
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
 * @note Only used for ::PselUiMode_SelectUser.
 */
NX_CONSTEXPR void pselUiSetAllowUserCreation(PselUiSettings *ui, bool flag) {
    if(ui->settings.mode == PselUiMode_UserSelector) {
        ui->settings.is_permitted = flag!=0;
    }
}

/**
 * @brief Sets whether users need to be linked to a Nintendo account.
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
NX_CONSTEXPR void pselUiSetNetworkServiceRequired(PselUiSettings *ui, bool flag) {
    ui->settings.is_network_service_account_required = flag;
}

/**
 * @brief Sets whether selection can be skipped with a button.
 * @param ui PselUiSettings struct.
 * @param flag Flag value.
 */
NX_CONSTEXPR void pselUiSetSkipButtonEnabled(PselUiSettings *ui, bool flag) {
    ui->settings.show_skip_button = flag!=0;
}

/**
 * @brief Shows the applet with the specified UI settings.
 * @param ui PselUiSettings struct.
 * @param out_user Selected user ID.
 * @note If user skips (see \ref pselUiSetSkipEnabled) this will return successfully but the output ID will be 0.
 */
Result pselUiShow(PselUiSettings *ui, AccountUid *out_user);

/**
 * @brief This is the System version of \ref pselShowUserSelector.
 * @note This uses \ref accountIsUserRegistrationRequestPermitted, hence \ref accountInitialize must be used prior to this. See also the docs for PselUserSelectionSettings::is_skip_enabled.
 * @param[out] out_user Returned selected user ID.
 * @param[in] settings \ref PselUserSelectionSettings
 * @param[in] settings_system [2.0.0+] \ref PselUserSelectionSettingsForSystemService, ignored on prior versions.
 */
Result pselShowUserSelectorForSystem(AccountUid *out_user, const PselUserSelectionSettings *settings, const PselUserSelectionSettingsForSystemService *settings_system);

/**
 * @brief This is the Launcher version of \ref pselShowUserSelector.
 * @note This uses \ref accountIsUserRegistrationRequestPermitted, hence \ref accountInitialize must be used prior to this. See also the docs for PselUserSelectionSettings::is_skip_enabled.
 * @param[out] out_user Returned selected user ID.
 * @param[in] settings \ref PselUserSelectionSettings
 * @param[in] application_id ApplicationId
 */
Result pselShowUserSelectorForLauncher(AccountUid *out_user, const PselUserSelectionSettings *settings, u64 application_id);

/**
 * @brief Shows the applet to select a user.
 * @note This uses \ref accountIsUserRegistrationRequestPermitted, hence \ref accountInitialize must be used prior to this. See also the docs for PselUserSelectionSettings::is_skip_enabled.
 * @param[out] out_user Returned selected user ID.
 * @param[in] settings \ref PselUserSelectionSettings
 */
Result pselShowUserSelector(AccountUid *out_user, const PselUserSelectionSettings *settings);

/**
 * @brief Shows the applet to create a user.
 * @note This uses \ref accountIsUserRegistrationRequestPermitted, hence \ref accountInitialize must be used prior to this. If the output flag is 0, an error will be thrown.
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
