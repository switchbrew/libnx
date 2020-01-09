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

Result capsaGetAlbumFileCount(CapsAlbumStorage storage, u64* count);
Result capsaGetAlbumFileList(CapsAlbumStorage storage, u64* count, CapsAlbumEntry* buffer, u64 size);
Result capsaLoadAlbumFile(const CapsAlbumFileId *file_id, u64 *out_size, void* workbuf, u64 workbuf_size);
Result capsaDeleteAlbumFile(const CapsAlbumFileId *file_id);
Result capsaStorageCopyAlbumFile(const CapsAlbumFileId *file_id, CapsAlbumStorage dst_storage);
Result capsaIsAlbumMounted(CapsAlbumStorage storage, bool* is_mounted);
Result capsaGetAlbumUsage(CapsAlbumStorage storage, CapsAlbumUsage2 *out);
Result capsaGetAlbumFileSize(const CapsAlbumFileId *file_id, u64* size);
Result capsaLoadAlbumFileThumbnail(const CapsAlbumFileId *file_id, u64 *out_size, void* workbuf, u64 workbuf_size);
/// Only available on [2.0.0+].
Result capsaLoadAlbumScreenShotImage(u64* width, u64* height, const CapsAlbumFileId *file_id, void* workbuf, u64 workbuf_size, void* rawbuf, u64 rawbuf_size);
Result capsaLoadAlbumScreenShotThumbnailImage(u64* width, u64* height, const CapsAlbumFileId *file_id, void* workbuf, u64 workbuf_size, void* rawbuf, u64 rawbuf_size);
/// Only available on [3.0.0+].
Result capsaLoadAlbumScreenShotImageEx(u64* width, u64* height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* workbuf, u64 workbuf_size, void* rawbuf, u64 rawbuf_size);
Result capsaLoadAlbumScreenShotThumbnailImageEx(u64* width, u64* height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* workbuf, u64 workbuf_size, void* rawbuf, u64 rawbuf_size);
/// Only available on [4.0.0+].
Result capsaGetAlbumUsage3(CapsAlbumStorage storage, CapsAlbumUsage3 *out);
Result capsaGetAlbumMountResult(CapsAlbumStorage storage);
Result capsaGetAlbumUsage16(CapsAlbumStorage storage, CapsAlbumUsage16 *out);

Result capsaGetAutoSavingStorage(CapsAlbumStorage* storage);
Result capsaGetRequiredStorageSpaceSizeToCopyAll(CapsAlbumStorage dst_storage, CapsAlbumStorage src_storage, u64* out);
/// Only available on [4.0.0+].
Result capsaLoadAlbumScreenShotThumbnailImageEx1(const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* workbuf, u64 workbuf_size, void* rawbuf, u64 rawbuf_size, void* out, u64 out_size);

Result capsaForceAlbumUnmounted(CapsAlbumStorage storage);
Result capsaResetAlbumMountStatus(CapsAlbumStorage storage);
Result capsaRefreshAlbumCache(CapsAlbumStorage storage);
