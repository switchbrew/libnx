/**
 * @file acc.h
 * @brief Account (acc:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "sm.h"

typedef struct {
    Service  s;
} AccountProfile;

typedef struct
{
    u32 unk_x0;
    u32 iconID;                ///< Icon ID. 0 = Mii, the rest are character icon IDs. 
    u8  iconBackgroundColorID; ///< Profile icon background color ID
    u8  unk_x9[0x7];
    u8  miiID[0x10];           ///< Some ID related to the Mii? All zeros when a character icon is used.
    u8 unk_x20[0x60];          ///< Usually zeros?
} PACKED AccountUserData;

typedef struct
{
    u128 userID;
    u64 lastEditTimestamp; ///< POSIX UTC timestamp, for the last account edit.
    char username[0x20];   ///< UTF-8 Username.
} PACKED AccountProfileBase;

Result accountInitialize(void);
void accountExit(void);
Service* accountGetService(void);

/// Get the userID for the currently active user. The output userID is only valid when the output account_selected==1, otherwise no user is currently selected.
/// An user is only selected when the user-account selection applet was used to select an user at least once before.
Result accountGetActiveUser(u128 *userID, bool *account_selected);

/// Get an AccountProfile for the specified userID.
Result accountGetProfile(AccountProfile* out, u128 userID);

/// Get \ref AccountUserData and \ref AccountProfileBase for the specified profile, userdata is optional (can be NULL).
Result accountProfileGet(AccountProfile* profile, AccountUserData* userdata, AccountProfileBase* profilebase);

/// Get the icon image size.
Result accountProfileGetImageSize(AccountProfile* profile, size_t* image_size);

/// Load the JPEG profile icon, valid for both Miis and character icons. The output image_size is the same as the one from \ref accountProfileGetImageSize.
Result accountProfileLoadImage(AccountProfile* profile, void* buf, size_t len, size_t* image_size);

void accountProfileClose(AccountProfile* profile);
