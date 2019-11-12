/**
 * @file friends_la.h
 * @brief Wrapper for using the MyPage (friends) LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/acc.h"
#include "../services/friends.h"

/// Arg type values used with \ref FriendsLaArg.
typedef enum {
    FriendsLaArgType_ShowFriendList                    = 0,      ///< ShowFriendList. Launches the applet with the "Friend List" menu initially selected.
    FriendsLaArgType_ShowUserDetailInfo                = 1,      ///< ShowUserDetailInfo
    FriendsLaArgType_StartSendingFriendRequest         = 2,      ///< StartSendingFriendRequest
    FriendsLaArgType_ShowMethodsOfSendingFriendRequest = 3,      ///< ShowMethodsOfSendingFriendRequest. Launches the applet with the "Add Friend" menu initially selected.
    FriendsLaArgType_StartFacedFriendRequest           = 4,      ///< StartFacedFriendRequest. Launches the applet where the "Search for Local Users" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowReceivedFriendRequestList     = 5,      ///< ShowReceivedFriendRequestList. Launches the applet where the "Received Friend Requests" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowBlockedUserList               = 6,      ///< ShowBlockedUserList. Launches the applet where the "Blocked-User List" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowMyProfile                     = 7,      ///< ShowMyProfile. Launches the applet with the "Profile" menu initially selected. ShowMyProfileForHomeMenu is identical to this except for playStartupSound=true.
    FriendsLaArgType_StartFriendInvitation             = 8,      ///< [9.0.0+] StartFriendInvitation. Launches the applet for sending online-play invites to friends, where the friends are selected via the UI.
    FriendsLaArgType_StartSendingFriendInvitation      = 9,      ///< [9.0.0+] StartSendingFriendInvitation.
    FriendsLaArgType_ShowReceivedInvitationDetail      = 10,     ///< [9.0.0+] ShowReceivedInvitationDetail.
} FriendsLaArgType;

/// Header for the arg struct.
typedef struct {
    u32 type;                                                    ///< \ref FriendsLaArgType
    u32 pad;                                                     ///< Padding.
    AccountUid uid;                                              ///< \ref AccountUid
} FriendsLaArgHeader;

/// Common data for the arg struct, for the pre-9.0.0 types.
/// This is only set for ::FriendsLaArgType_ShowUserDetailInfo/::FriendsLaArgType_StartSendingFriendRequest, for everything else this is cleared.
typedef struct {
    AccountNetworkServiceAccountId id;                           ///< \ref AccountNetworkServiceAccountId for the other account.
    FriendsInAppScreenName first_inAppScreenName;                ///< First InAppScreenName.
    FriendsInAppScreenName second_inAppScreenName;               ///< Second InAppScreenName.
} FriendsLaArgCommonData;

/// Arg struct pushed for the applet input storage, for pre-9.0.0.
typedef struct {
    FriendsLaArgHeader hdr;                                      ///< \ref FriendsLaArgHeader
    FriendsLaArgCommonData data;                                 ///< \ref FriendsLaArgCommonData
} FriendsLaArgV1;

/// Arg struct pushed for the applet input storage, for [9.0.0+].
typedef struct {
    FriendsLaArgHeader hdr;                                      ///< \ref FriendsLaArgHeader

    union {
        u8 raw[0x1090];                                          ///< Raw data.

        FriendsLaArgCommonData common;                           ///< \ref FriendsLaArgCommonData

        struct {
            s32 id_count;                                        ///< \ref AccountNetworkServiceAccountId count, must be 1-15.
            u32 pad;                                             ///< Padding.
            u64 userdata_size;                                   ///< User-data size, must be <=0x400.
            u8 userdata[0x400];                                  ///< Arbitrary user-data, see above size.
            FriendsFriendInvitationGameModeDescription desc;     ///< \ref FriendsFriendInvitationGameModeDescription
        } start_friend_invitation;                               ///< Data for ::FriendsLaArgType_StartFriendInvitation.

        struct {
            s32 id_count;                                        ///< \ref AccountNetworkServiceAccountId count, must be 1-15.
            u32 pad;                                             ///< Padding.
            AccountNetworkServiceAccountId id_list[16];          ///< \ref AccountNetworkServiceAccountId list, see above count.
            u64 userdata_size;                                   ///< User-data size, must be <=0x400.
            u8 userdata[0x400];                                  ///< Arbitrary user-data, see above size.
            FriendsFriendInvitationGameModeDescription desc;     ///< \ref FriendsFriendInvitationGameModeDescription
        } start_sending_friend_invitation;                       ///< Data for ::FriendsLaArgType_StartSendingFriendInvitation.

        struct {
            FriendsFriendInvitationId invitation_id;             ///< \ref FriendsFriendInvitationId
            FriendsFriendInvitationGroupId invitation_group_id;  ///< \ref FriendsFriendInvitationGroupId
        } show_received_invitation_detail;                       ///< Data for ::FriendsLaArgType_ShowReceivedInvitationDetail.
    } data;                                                      ///< Data for each \ref FriendsLaArgType.
} FriendsLaArg;

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowFriendList, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowFriendList(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowUserDetailInfo, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 * @param[in] id \ref AccountNetworkServiceAccountId for the user to show UserDetailInfo for.
 * @param[in] first_inAppScreenName First \ref FriendsInAppScreenName.
 * @param[in] second_inAppScreenName Second \ref FriendsInAppScreenName.
 */
Result friendsLaShowUserDetailInfo(AccountUid uid, AccountNetworkServiceAccountId id, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartSendingFriendRequest, the specified input, and playStartupSound=false. On success, this will load the output Result from the output storage.
 * @param[in] uid \ref AccountUid
 * @param[in] id \ref AccountNetworkServiceAccountId to send the friend request to.
 * @param[in] first_inAppScreenName First \ref FriendsInAppScreenName.
 * @param[in] second_inAppScreenName Second \ref FriendsInAppScreenName.
 */
Result friendsLaStartSendingFriendRequest(AccountUid uid, AccountNetworkServiceAccountId id, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowMethodsOfSendingFriendRequest, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowMethodsOfSendingFriendRequest(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartFacedFriendRequest, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaStartFacedFriendRequest(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowReceivedFriendRequestList, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowReceivedFriendRequestList(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowBlockedUserList, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowBlockedUserList(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowMyProfile, the specified input, and playStartupSound=false.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowMyProfile(AccountUid uid);

/**
 * @brief Same as \ref friendsLaShowMyProfile except with playStartupSound=true.
 * @param[in] uid \ref AccountUid
 */
Result friendsLaShowMyProfileForHomeMenu(AccountUid uid);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartFriendInvitation, the specified input, and playStartupSound=false. On success, this will load the output Result from the output storage.
 * @note Only available on [9.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[in] id_count \ref AccountNetworkServiceAccountId count, must be 1-15. Number of friends to invite.
 * @param[in] desc \ref FriendsFriendInvitationGameModeDescription
 * @param[in] userdata Arbitrary user-data. Can be NULL.
 * @param[in] userdata_size User-data size, must be <=0x400. Can be 0 if userdata is NULL.
 */
Result friendsLaStartFriendInvitation(AccountUid uid, s32 id_count, const FriendsFriendInvitationGameModeDescription *desc, const void* userdata, u64 userdata_size);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartSendingFriendInvitation, the specified input, and playStartupSound=false. On success, this will load the output Result from the output storage.
 * @note Only available on [9.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[in] id_list \ref AccountNetworkServiceAccountId list.
 * @param[in] id_count Size of the id_list array in entries, must be 1-15. Number of friends to invite.
 * @param[in] desc \ref FriendsFriendInvitationGameModeDescription
 * @param[in] userdata Arbitrary user-data. Can be NULL.
 * @param[in] userdata_size User-data size, must be <=0x400. Can be 0 if userdata is NULL.
 */
Result friendsLaStartSendingFriendInvitation(AccountUid uid, const AccountNetworkServiceAccountId *id_list, s32 id_count, const FriendsFriendInvitationGameModeDescription *desc, const void* userdata, u64 userdata_size);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowReceivedInvitationDetail, the specified input, and playStartupSound=false.
 * @note Only available on [9.0.0+].
 * @param[in] uid \ref AccountUid
 * @param[in] invitation_id \ref FriendsFriendInvitationId
 * @param[in] invitation_group_id \ref FriendsFriendInvitationGroupId
 */
Result friendsLaShowReceivedInvitationDetail(AccountUid uid, FriendsFriendInvitationId invitation_id, FriendsFriendInvitationGroupId invitation_group_id);

