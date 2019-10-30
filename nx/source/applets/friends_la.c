#include <string.h>
#include "types.h"
#include "result.h"
#include "services/acc.h"
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

static Result _friendsLaShowSimple(FriendsLaArgType type, AccountUid uid, bool playStartupSound) {
    FriendsLaArg arg = {.type = type, .uid = uid};

    return _friendsLaShow(&arg, playStartupSound);
}

static Result _friendsLaShowAll(FriendsLaArgType type, AccountUid uid, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName, bool playStartupSound) {
    FriendsLaArg arg = {.type = type, .uid = uid, .networkServiceAccountId = networkServiceAccountId, .first_inAppScreenName = *first_inAppScreenName, .second_inAppScreenName = *second_inAppScreenName};

    return _friendsLaShow(&arg, playStartupSound);
}

Result friendsLaShowFriendList(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowFriendList, uid, false);
}

Result friendsLaShowUserDetailInfo(AccountUid uid, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_ShowUserDetailInfo, uid, networkServiceAccountId, first_inAppScreenName, second_inAppScreenName, false);
}

Result friendsLaStartSendingFriendRequest(AccountUid uid, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_StartSendingFriendRequest, uid, networkServiceAccountId, first_inAppScreenName, second_inAppScreenName, false);
}

Result friendsLaShowMethodsOfSendingFriendRequest(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMethodsOfSendingFriendRequest, uid, false);
}

Result friendsLaStartFacedFriendRequest(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_StartFacedFriendRequest, uid, false);
}

Result friendsLaShowReceivedFriendRequestList(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowReceivedFriendRequestList, uid, false);
}

Result friendsLaShowBlockedUserList(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowBlockedUserList, uid, false);
}

Result friendsLaShowMyProfile(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMyProfile, uid, false);
}

Result friendsLaShowMyProfileForHomeMenu(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowMyProfile, uid, true);
}

