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
    AccountServiceType_NotInitialized = 0,  ///< Same as ::AccountServiceType_Application during \ref accountInitialize.
    AccountServiceType_Application    = 1,  ///< Initializes acc:u0.
    AccountServiceType_System         = 2,  ///< Initializes acc:u1.
    AccountServiceType_Administrator  = 3,  ///< Initializes acc:su.
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
    u8  iconBackgroundColorID; ///< Profile icon background color ID
    u8  unk_x9[0x7];           ///< Unknown.
    u8  miiID[0x10];           ///< Some ID related to the Mii? All zeros when a character icon is used.
    u8 unk_x20[0x60];          ///< Usually zeros?
} AccountUserData;

/// ProfileBase
typedef struct {
    AccountUid userID;     ///< \ref AccountUid
    u64 lastEditTimestamp; ///< POSIX UTC timestamp, for the last account edit.
    char nickname[0x20];   ///< UTF-8 Nickname.
} AccountProfileBase;

/**
 * @brief Sets the \ref AccountServiceType for initialization. Call this function before \ref accountInitialize, if needed.
 * @note By default ::AccountServiceType_NotInitialized will be used.
 */
void accountSetServiceType(AccountServiceType serviceType);

/// Initialize account.
Result accountInitialize(void);

/// Exit account.
void accountExit(void);

/// Gets the Service object for the actual account service session.
Service* accountGetServiceSession(void);

/// Get the total number of user profiles.
Result accountGetUserCount(s32* user_count);

/**
 * @brief Get a list of all user IDs. The returned list will never be larger than ACC_USER_LIST_SIZE.
 * @param userIDs Pointer to array of user IDs.
 * @param max_userIDs Maximum number of user IDs to return.
 * @param actual_total The actual total number of user IDs found.
 */
Result accountListAllUsers(AccountUid* userIDs, s32 max_userIDs, s32 *actual_total);

/// Get the userID for the last opened user.
Result accountGetLastOpenedUser(AccountUid *userID);

/// Get an AccountProfile for the specified userID.
Result accountGetProfile(AccountProfile* out, AccountUid userID);

/// Close the AccountProfile.
void accountProfileClose(AccountProfile* profile);

/// Get \ref AccountUserData and \ref AccountProfileBase for the specified profile, userdata is optional (can be NULL).
Result accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase);

/// Get the icon image size.
Result accountProfileGetImageSize(AccountProfile* profile, u32* image_size);

/// Load the JPEG profile icon, valid for both Miis and character icons. The output image_size is the same as the one from \ref accountProfileGetImageSize.
Result accountProfileLoadImage(AccountProfile* profile, void* buf, size_t len, u32* image_size);

/// Gets the userID which was selected by the profile-selector applet (if any), prior to launching the currently running Application title.
/// This gets the cached PreselectedUser loaded during accountInitialize, when PreselectedUser is available.
Result accountGetPreselectedUser(AccountUid *userID);

/**
 * @brief Checks whether the specified \ref AccountUid is valid/set (non-zero).
 * @param[in] Uid \ref AccountUid
 */
NX_CONSTEXPR bool accountUidIsValid(const AccountUid *Uid) {
    return Uid->uid[0]!=0 || Uid->uid[1]!=0;
}

