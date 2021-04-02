#include "service_guard.h"
#include "services/friends.h"

Result friendsTryPopFriendInvitationNotificationInfo(AccountUid *uid, void* buffer, u64 size, u64 *out_size) {
    Result rc=0;
    AppletStorage storage;
    s64 storage_size=0;
    u64 data_size = size;
    AccountUid tmpuid={0};

    rc = appletTryPopFromFriendInvitationStorageChannel(&storage);
    if (R_SUCCEEDED(rc)) rc = appletStorageGetSize(&storage, &storage_size);
    if (R_SUCCEEDED(rc) && storage_size < sizeof(AccountUid)) rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (R_SUCCEEDED(rc)) {
        storage_size-=sizeof(AccountUid);
        rc = appletStorageRead(&storage, 0, &tmpuid, sizeof(AccountUid));
        if (R_SUCCEEDED(rc)) {
            if (data_size > storage_size) data_size = storage_size;
            if (data_size) rc = appletStorageRead(&storage, sizeof(AccountUid), buffer, data_size);
            if (R_SUCCEEDED(rc)) {
                if (out_size) *out_size = data_size;
                if (uid) *uid = tmpuid;
            }
        }
    }

    appletStorageClose(&storage);
    return rc;
}

