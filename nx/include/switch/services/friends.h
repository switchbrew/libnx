/**
 * @file friend.h
 * @brief Friends (friend:*) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"

/// InAppScreenName
typedef struct {
    char name[0x40];                   ///< UTF-8 string, NUL-terminated.
    u64 languageCode;                  ///< LanguageCode, see set.h.
} FriendsInAppScreenName;

/// FriendInvitationGameModeDescription
typedef struct {
    u8 unk_x0[0xc00];                  ///< Unknown.
} FriendsFriendInvitationGameModeDescription;

/// FriendInvitationId
typedef struct {
    u64 id;                            ///< Id.
} FriendsFriendInvitationId;

/// FriendInvitationGroupId
typedef struct {
    u64 id;                            ///< Id.
} FriendsFriendInvitationGroupId;

