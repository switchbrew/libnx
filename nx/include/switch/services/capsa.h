/**
 * @file capsa.h
 * @brief Album Accessor (caps:a) service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"

/// Initialize caps:a.
Result capsaInitialize(void);

/// Exit caps:a.
void capsaExit(void);

/// Gets the Service for caps:a.
Service* capsaGetServiceSession(void);
// 0-8
Result capsaGetAlbumFileCount(CapsAlbumStorage storage, u64* count);
Result capsaGetAlbumFileList(CapsAlbumStorage storage, u64* count, CapsApplicationAlbumEntry* buffer, u64 size);
Result capsaLoadAlbumFile(CapsAlbumFileId file_id, u64 *out_size, void* jpeg_buffer, u64 jpeg_buffer_size);
Result capsaDeleteAlbumFile(CapsAlbumFileId file_id);
Result capsaStorageCopyAlbumFile(CapsAlbumFileId file_id, CapsAlbumStorage dst_storage);
Result capsaIsAlbumMounted(CapsAlbumStorage storage, bool* is_mounted);
Result capsaGetAlbumUsage(CapsAlbumStorage storage, CapsAlbumUsage2 *out);
Result capsaGetAlbumFileSize(CapsAlbumFileId file_id, u64* size);
Result capsaLoadAlbumFileThumbnail(CapsAlbumFileId file_id, u64 *out_size, void* jpeg_buffer, u64 jpeg_buffer_size);
// 15-18
Result capsaGetAlbumUsage3(CapsAlbumStorage storage, CapsAlbumUsage3 *out);
Result capsaGetAlbumMountResult(CapsAlbumStorage storage);
Result capsaGetAlbumUsage16(CapsAlbumStorage storage, CapsAlbumUsage16 *out);

Result capsaGetAutoSavingStorage(CapsAlbumStorage* storage);
Result capsaGetRequiredStorageSpaceSizeToCopyAll(CapsAlbumStorage dst_storage, CapsAlbumStorage src_storage, u64* out);

Result capsaLoadAlbumScreenShotThumbnailImage(u64* width, u64* height, CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size);
Result capsaLoadAlbumScreenShotThumbnailImageEx(u64* width, u64* height, CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size);
Result capsaLoadAlbumScreenShotThumbnailImageEx1(CapsAlbumFileId file_id, void* jpeg_buffer, u64 jpeg_buffer_size, void* buffer, u64 buffer_size, void* out, u64 out_size);

Result capsaForceAlbumUnmounted(CapsAlbumStorage storage);
Result capsaResetAlbumMountStatus(CapsAlbumStorage storage);
Result capsaRefreshAlbumCache(CapsAlbumStorage storage);
