#include <string.h>
#include "types.h"
#include "result.h"
#include "services/applet.h"
#include "applets/libapplet.h"
#include "applets/friends_la.h"

static Result _friendsLaShow(const FriendsLaArg *arg, bool playStartupSound) {
    Result rc=0;
    Result rc2=0;
    size_t readsize=0;
    LibAppletArgs commonargs;

    libappletArgsCreate(&commonargs, 0x1);
    libappletArgsSetPlayStartupSound(&commonargs, playStartupSound);

    if (arg->type != FriendsLaArgType_StartSendingFriendRequest)
        rc = libappletLaunch(AppletId_myPage, &commonargs, arg, sizeof(*arg), NULL, 0, NULL);
    else {
        rc = libappletLaunch(AppletId_myPage, &commonargs, arg, sizeof(*arg), &rc2, sizeof(rc2), &readsize);
        if (R_SUCCEEDED(rc) && readsize!=sizeof(rc2)) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (R_SUCCEEDED(rc)) rc = rc2;
    }

    return rc;
}

static Result _friendsLaShowSimple(FriendsLaArgType type, u128 userID, bool playStartupSound) {
    FriendsLaArg arg = {.type = type, .userID = userID};

    return _friendsLaShow(&arg, playStartupSound);
}

static Result _friendsLaShowAll(FriendsLaArgType type, u128 userID, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName, bool playStartupSound) {
    FriendsLaArg arg = {.type = type, .userID = userID, .networkServiceAccountId = networkServiceAccountId, .first_inAppScreenName = *first_inAppScreenName, .second_inAppScreenName = *second_inAppScreenName};

    return _friendsLaShow(&arg, playStartupSound);
}

Result friendsLaShowFriendList(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowFriendList, userID, false);
}

Result friendsLaShowUserDetailInfo(u128 userID, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_ShowUserDetailInfo, userID, networkServiceAccountId, first_inAppScreenName, second_inAppScreenName, false);
}

Result friendsLaStartSendingFriendRequest(u128 userID, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_StartSendingFriendRequest, userID, networkServiceAccountId, first_inAppScreenName, second_inAppScreenName, false);
}

Result friendsLaShowMethodsOfSendingFriendRequest(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMethodsOfSendingFriendRequest, userID, false);
}

Result friendsLaStartFacedFriendRequest(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_StartFacedFriendRequest, userID, false);
}

Result friendsLaShowReceivedFriendRequestList(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowReceivedFriendRequestList, userID, false);
}

Result friendsLaShowBlockedUserList(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowBlockedUserList, userID, false);
}

Result friendsLaShowMyProfile(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMyProfile, userID, false);
}

Result friendsLaShowMyProfileForHomeMenu(u128 userID) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMyProfile, userID, true);
}

