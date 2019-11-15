#include "service_guard.h"
#include "services/notif.h"

Result notifTryPopNotifiedApplicationParameter(void* buffer, u64 size, u64 *out_size) {
    Result rc=0;
    AppletStorage storage;
    s64 storage_size=0;
    u64 data_size = size;

    rc = appletTryPopFromNotificationStorageChannel(&storage);
    if (R_SUCCEEDED(rc)) rc = appletStorageGetSize(&storage, &storage_size);
    if (R_SUCCEEDED(rc)) {
        if (data_size > storage_size) data_size = storage_size;
        if (data_size) rc = appletStorageRead(&storage, 0, buffer, data_size);
        if (R_SUCCEEDED(rc) && out_size) *out_size = data_size;
    }

    appletStorageClose(&storage);
    return rc;
}

