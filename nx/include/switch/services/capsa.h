/**
 * @file capsu.h
 * @brief Album Accessor (caps:a) service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"

typedef enum {
    AlbumObjectLocation_BuiltInUser = 0,
    AlbumObjectLocation_SdCard      = 1,
} AlbumObjectLocation;

/// Initialize caps:a.
Result capsaInitialize(void);

/// Exit caps:a.
void capsaExit(void);

/// Gets the Service for caps:u.
Service* capsuGetServiceSession(void);

Result capsaGetAlbumFileCount(AlbumObjectLocation location, u64* count);
Result capsaGetAlbumFileList(AlbumObjectLocation location, u64* count, CapsApplicationAlbumEntry* buffer, u64 size);
Result capsaLoadAlbumFile(CapsAlbumEntryId entry_id, u8 unk[8], void* jpeg_buffer, u64 jpeg_buffer_size);
Result capsaDeleteAlbumFile(CapsAlbumEntryId entry_id);
Result capsaStorageCopyAlbumFile(u8 unk[0x20]);
Result capsaIsAlbumMounted(AlbumObjectLocation location, bool* is_mounted);
Result capsaGetAlbumUsage(AlbumObjectLocation location, u8 unk_out[0x30]);
Result capsaGetAlbumFileSize(CapsAlbumEntryId entry_id, u64* size);
Result capsLoadAlbumFileThumbnail(CapsAlbumEntryId entry_id, u8 unk[8], void* jpeg_buffer, u64 jpeg_buffer_size);
