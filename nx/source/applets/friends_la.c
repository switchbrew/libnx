#include <string.h>
#include "libapplet_internal.h"
#include "applets/friends_la.h"
#include "runtime/hosversion.h"

static Result _friendsLaShow(const FriendsLaArg *arg, bool playStartupSound) {
    Result rc=0;
    Result rc2=0;
    size_t readsize=0;
    u32 version=0x1;
    const void* arg_ptr = arg;
    size_t arg_size = sizeof(*arg);
    LibAppletArgs commonargs;
    FriendsLaArgV1 argv1;

    if (hosversionAtLeast(9,0,0))
        version = 0x10000;
    else {
        memset(&argv1, 0, sizeof(argv1));
        arg_ptr = &argv1;
        arg_size = sizeof(argv1);

        memcpy(&argv1.hdr, &arg->hdr, sizeof(FriendsLaArgHeader));
        memcpy(&argv1.data, &arg->data.common, sizeof(FriendsLaArgCommonData));
    }

    libappletArgsCreate(&commonargs, version);
    libappletArgsSetPlayStartupSound(&commonargs, playStartupSound);

    if (arg->hdr.type != FriendsLaArgType_StartSendingFriendRequest && arg->hdr.type != FriendsLaArgType_StartFriendInvitation && arg->hdr.type != FriendsLaArgType_StartSendingFriendInvitation)
        rc = libappletLaunch(AppletId_myPage, &commonargs, arg_ptr, arg_size, NULL, 0, NULL);
    else {
        rc = libappletLaunch(AppletId_myPage, &commonargs, arg_ptr, arg_size, &rc2, sizeof(rc2), &readsize);
        if (R_SUCCEEDED(rc) && readsize!=sizeof(rc2)) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        if (R_SUCCEEDED(rc)) rc = rc2;
    }

    return rc;
}

static Result _friendsLaShowSimple(FriendsLaArgType type, AccountUid uid, bool playStartupSound) {
    FriendsLaArg arg = {.hdr.type = type, .hdr.uid = uid};

    return _friendsLaShow(&arg, playStartupSound);
}

static Result _friendsLaShowAll(FriendsLaArgType type, AccountUid uid, AccountNetworkServiceAccountId id, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName, bool playStartupSound) {
    FriendsLaArg arg = {.hdr.type = type, .hdr.uid = uid, .data.common.id = id, .data.common.first_inAppScreenName = *first_inAppScreenName, .data.common.second_inAppScreenName = *second_inAppScreenName};

    return _friendsLaShow(&arg, playStartupSound);
}

Result friendsLaShowFriendList(AccountUid uid) {
    return _friendsLaShowSimple(FriendsLaArgType_ShowFriendList, uid, false);
}

Result friendsLaShowUserDetailInfo(AccountUid uid, AccountNetworkServiceAccountId id, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_ShowUserDetailInfo, uid, id, first_inAppScreenName, second_inAppScreenName, false);
}

Result friendsLaStartSendingFriendRequest(AccountUid uid, AccountNetworkServiceAccountId id, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName) {
    return _friendsLaShowAll(FriendsLaArgType_StartSendingFriendRequest, uid, id, first_inAppScreenName, second_inAppScreenName, false);
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

Result friendsLaStartFriendInvitation(AccountUid uid, s32 id_count, const FriendsFriendInvitationGameModeDescription *desc, const void* userdata, u64 userdata_size) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    FriendsLaArg arg;
    memset(&arg, 0, sizeof(arg));
    if (id_count<1 || id_count>15 || userdata_size>=sizeof(arg.data.start_friend_invitation.userdata) || (userdata==NULL && userdata_size!=0))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    arg.hdr.type = FriendsLaArgType_StartFriendInvitation;
    arg.hdr.uid = uid;

    arg.data.start_friend_invitation.id_count = id_count;
    arg.data.start_friend_invitation.userdata_size = userdata_size;
    if (userdata && userdata_size) memcpy(arg.data.start_friend_invitation.userdata, userdata, userdata_size);
    memcpy(&arg.data.start_friend_invitation.desc, desc, sizeof(*desc));

    return _friendsLaShow(&arg, false);
}

Result friendsLaStartSendingFriendInvitation(AccountUid uid, const AccountNetworkServiceAccountId *id_list, s32 id_count, const FriendsFriendInvitationGameModeDescription *desc, const void* userdata, u64 userdata_size) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    FriendsLaArg arg;
    memset(&arg, 0, sizeof(arg));
    if (id_count<1 || id_count>15 || userdata_size>=sizeof(arg.data.start_sending_friend_invitation.userdata) || (userdata==NULL && userdata_size!=0))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    arg.hdr.type = FriendsLaArgType_StartSendingFriendInvitation;
    arg.hdr.uid = uid;

    arg.data.start_sending_friend_invitation.id_count = id_count;
    arg.data.start_sending_friend_invitation.userdata_size = userdata_size;
    memcpy(arg.data.start_sending_friend_invitation.id_list, id_list, id_count*sizeof(AccountNetworkServiceAccountId));
    if (userdata && userdata_size) memcpy(arg.data.start_sending_friend_invitation.userdata, userdata, userdata_size);
    memcpy(&arg.data.start_sending_friend_invitation.desc, desc, sizeof(*desc));

    return _friendsLaShow(&arg, false);
}

Result friendsLaShowReceivedInvitationDetail(AccountUid uid, FriendsFriendInvitationId invitation_id, FriendsFriendInvitationGroupId invitation_group_id) {
    if (hosversionBefore(9,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    FriendsLaArg arg;
    memset(&arg, 0, sizeof(arg));

    arg.hdr.type = FriendsLaArgType_ShowReceivedInvitationDetail;
    arg.hdr.uid = uid;

    arg.data.show_received_invitation_detail.invitation_id = invitation_id;
    arg.data.show_received_invitation_detail.invitation_group_id = invitation_group_id;

    return _friendsLaShow(&arg, false);
}

