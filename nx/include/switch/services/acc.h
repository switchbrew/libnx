/**
 * @file acc.h
 * @brief Account (acc:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

#define ACC_USER_LIST_SIZE 8

typedef enum {
    AccountServiceType_Application    = 0,  ///< Initializes acc:u0.
    AccountServiceType_System         = 1,  ///< Initializes acc:u1.
    AccountServiceType_Administrator  = 2,  ///< Initializes acc:su.
} AccountServiceType;

/// Profile
typedef struct {
    Service  s;       ///< IProfile
} AccountProfile;

/// Account UserId.
typedef struct {
    u64 uid[2];       ///< UserId. All-zero is invalid / Uid not set. See also \ref accountUidIsValid.
} AccountUid;

/// UserData
typedef struct {
    u32 unk_x0;                ///< Unknown.
    u32 iconID;                ///< Icon ID. 0 = Mii, the rest are character icon IDs.
    u8 iconBackgroundColorID;  ///< Profile icon background color ID
    u8 unk_x9[0x7];            ///< Unknown.
    u8 miiID[0x10];            ///< Some ID related to the Mii? All zeros when a character icon is used.
    u8 unk_x20[0x60];          ///< Usually zeros?
} AccountUserData;

/// ProfileBase
typedef struct {
    AccountUid uid;        ///< \ref AccountUid
    u64 lastEditTimestamp; ///< POSIX UTC timestamp, for the last account edit.
    char nickname[0x20];   ///< UTF-8 Nickname.
} AccountProfileBase;

/// NetworkServiceAccountId
typedef struct {
    u64 id;                ///< Id.
} AccountNetworkServiceAccountId;

/// Initialize account.
Result accountInitialize(AccountServiceType service_type);

/// Exit account.
void accountExit(void);

/// Gets the Service object for the actual account service session.
Service* accountGetServiceSession(void);

/// Get the total number of user profiles.
Result accountGetUserCount(s32* user_count);

/**
 * @brief Get a list of all userIds. The returned list will never be larger than ACC_USER_LIST_SIZE.
 * @param uids Pointer to array of userIds.
 * @param max_uids Maximum number of userIds to return.
 * @param actual_total The actual total number of userIds found.
 */
Result accountListAllUsers(AccountUid* uids, s32 max_uids, s32 *actual_total);

/// Get the userId for the last opened user.
Result accountGetLastOpenedUser(AccountUid *uid);

/// Get an AccountProfile for the specified userId.
Result accountGetProfile(AccountProfile* out, AccountUid uid);

/// IsUserRegistrationRequestPermitted
Result accountIsUserRegistrationRequestPermitted(bool *out);

/// TrySelectUserWithoutInteraction
Result accountTrySelectUserWithoutInteraction(AccountUid *uid, bool is_network_service_account_required);

/// Close the AccountProfile.
void accountProfileClose(AccountProfile* profile);

/// Get \ref AccountUserData and \ref AccountProfileBase for the specified profile, userdata is optional (can be NULL).
Result accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase);

/// Get the icon image size.
Result accountProfileGetImageSize(AccountProfile* profile, u32* image_size);

/// Load the JPEG profile icon, valid for both Miis and character icons. The output image_size is the same as the one from \ref accountProfileGetImageSize.
Result accountProfileLoadImage(AccountProfile* profile, void* buf, size_t len, u32* image_size);

/// Gets the userId which was selected by the profile-selector applet (if any), prior to launching the currently running Application.
/// This gets the cached PreselectedUser loaded during accountInitialize, when PreselectedUser is available.
Result accountGetPreselectedUser(AccountUid *uid);

/**
 * @brief Checks whether the specified \ref AccountUid is valid/set (non-zero).
 * @param[in] Uid \ref AccountUid
 */
NX_CONSTEXPR bool accountUidIsValid(const AccountUid *Uid) {
    return Uid->uid[0]!=0 || Uid->uid[1]!=0;
}

