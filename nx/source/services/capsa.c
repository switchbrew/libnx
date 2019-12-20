#define NX_SERVICE_ASSUME_NON_DOMAIN
#include <string.h>
#include <time.h>
#include "service_guard.h"
#include "runtime/hosversion.h"
#include "services/applet.h"
#include "services/capsa.h"

static Service g_capsaSrv;

NX_GENERATE_SERVICE_GUARD(capsa);

Result _capsaInitialize(void) {
    return smGetService(&g_capsaSrv, "caps:a");
}

void _capsaCleanup(void) {
    serviceClose(&g_capsaSrv);
}

Service* capsaGetServiceSession(void) {
    return &g_capsaSrv;
}

Result capsaGetAlbumFileCount(AlbumObjectLocation location, u64* count) {
    return serviceDispatchInOut(&g_capsaSrv, 0, location, *count);
}

Result capsaGetAlbumFileList(AlbumObjectLocation location, u64* count, CapsApplicationAlbumEntry* buffer, u64 buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 1, location, *count,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, buffer_size } },
    );
}

Result capsaLoadAlbumFile(CapsAlbumEntryId entry_id, u8 unk[8], void* jpeg_buffer, u64 jpeg_buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 2, entry_id, *unk,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { jpeg_buffer, jpeg_buffer_size } },
    );
}

Result capsaDeleteAlbumFile(CapsAlbumEntryId entry_id) {
    return serviceDispatchIn(&g_capsaSrv, 3, entry_id);
}

Result capsaStorageCopyAlbumFile(u8 unk[0x20]) {
    return serviceDispatchIn(&g_capsaSrv, 4, *unk);
}

Result capsaIsAlbumMounted(AlbumObjectLocation location, bool* is_mounted) {
    return serviceDispatchInOut(&g_capsaSrv, 5, location, *is_mounted);
}

Result capsaGetAlbumUsage(AlbumObjectLocation location, u8 unk_out[0x30]) {
    return serviceDispatchInOut(&g_capsaSrv, 6, location, *unk_out);
}

Result capsaGetAlbumFileSize(CapsAlbumEntryId entry_id, u64* size) {
    return serviceDispatchInOut(&g_capsaSrv, 7, entry_id, *size);
}

Result capsaLoadAlbumFileThumbnail(CapsAlbumEntryId entry_id, u8 unk[8], void* jpeg_buffer, u64 jpeg_buffer_size) {
    return serviceDispatchInOut(&g_capsaSrv, 8, entry_id, *unk,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { jpeg_buffer, jpeg_buffer_size } },
    );
}
