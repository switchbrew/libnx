/**
 * @file friends_la.h
 * @brief Wrapper for using the MyPage (friends) LibraryApplet.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../services/friends.h"

/// Arg type values used with \ref FriendsLaArg.
typedef enum {
    FriendsLaArgType_ShowFriendList                    = 0,   ///< ShowFriendList. Launches the applet with the "Friend List" menu initially selected.
    FriendsLaArgType_ShowUserDetailInfo                = 1,   ///< ShowUserDetailInfo
    FriendsLaArgType_StartSendingFriendRequest         = 2,   ///< StartSendingFriendRequest
    FriendsLaArgType_ShowMethodsOfSendingFriendRequest = 3,   ///< ShowMethodsOfSendingFriendRequest. Launches the applet with the "Add Friend" menu initially selected.
    FriendsLaArgType_StartFacedFriendRequest           = 4,   ///< StartFacedFriendRequest. Launches the applet where the "Search for Local Users" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowReceivedFriendRequestList     = 5,   ///< ShowReceivedFriendRequestList. Launches the applet where the "Received Friend Requests" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowBlockedUserList               = 6,   ///< ShowBlockedUserList. Launches the applet where the "Blocked-User List" menu is initially shown. Returning from this menu will exit the applet.
    FriendsLaArgType_ShowMyProfile                     = 7,   ///< ShowMyProfile. Launches the applet with the "Profile" menu initially selected. ShowMyProfileForHomeMenu is identical to this except for playStartupSound=true.
} FriendsLaArgType;

/// Arg struct pushed for the applet input storage.
/// The fields following the userID are only set for ::FriendsLaArgType_ShowUserDetailInfo/::FriendsLaArgType_StartSendingFriendRequest, for everything else these are cleared.
typedef struct {
    u32 type;                                                 ///< \ref FriendsLaArgType
    u32 pad;                                                  ///< Padding.
    union { u128 userID; } PACKED;                            ///< Account userID.
    u64 networkServiceAccountId;                              ///< NetworkServiceAccountId for the other account.
    FriendsInAppScreenName first_inAppScreenName;             ///< First InAppScreenName.
    FriendsInAppScreenName second_inAppScreenName;            ///< Second InAppScreenName.
} FriendsLaArg;

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowFriendList, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaShowFriendList(u128 userID);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowUserDetailInfo, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 * @param[in] networkServiceAccountId NetworkServiceAccountId for the user to show UserDetailInfo for.
 * @param[in] first_inAppScreenName First \ref FriendsInAppScreenName.
 * @param[in] second_inAppScreenName Second \ref FriendsInAppScreenName.
 */
Result friendsLaShowUserDetailInfo(u128 userID, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartSendingFriendRequest, the specified input, and playStartupSound=false. On success, this will load the output Result from the output storage.
 * @param[in] userID Account userID.
 * @param[in] networkServiceAccountId NetworkServiceAccountId to send the friend request to.
 * @param[in] first_inAppScreenName First \ref FriendsInAppScreenName.
 * @param[in] second_inAppScreenName Second \ref FriendsInAppScreenName.
 */
Result friendsLaStartSendingFriendRequest(u128 userID, u64 networkServiceAccountId, const FriendsInAppScreenName *first_inAppScreenName, const FriendsInAppScreenName *second_inAppScreenName);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowMethodsOfSendingFriendRequest, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaShowMethodsOfSendingFriendRequest(u128 userID);

/**
 * @brief Launches the applet with ::FriendsLaArgType_StartFacedFriendRequest, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaStartFacedFriendRequest(u128 userID);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowReceivedFriendRequestList, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaShowReceivedFriendRequestList(u128 userID);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowBlockedUserList, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaShowBlockedUserList(u128 userID);

/**
 * @brief Launches the applet with ::FriendsLaArgType_ShowMyProfile, the specified input, and playStartupSound=false.
 * @param[in] userID Account userID.
 */
Result friendsLaShowMyProfile(u128 userID);

/**
 * @brief Same as \ref friendsLaShowMyProfile except with playStartupSound=true.
 * @param[in] userID Account userID.
 */
Result friendsLaShowMyProfileForHomeMenu(u128 userID);

