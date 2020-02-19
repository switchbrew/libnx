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

/// Gets the Service for IAlbumAccessorSession, only initialized after \ref capsaOpenAlbumMovieStream was used (unaffected by using \ref capsaCloseAlbumMovieStream).
Service* capsaGetServiceSession_Accessor(void);

/**
 * @brief Gets the amount of files at a AlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] count Amount of files.
 */
Result capsaGetAlbumFileCount(CapsAlbumStorage storage, u64 *count);

/**
 * @brief Gets a listing of \ref CapsAlbumEntry, where the AlbumFile's storage matches the input one.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] out Total output entries.
 * @param[out] entries Output array of \ref CapsAlbumEntry.
 * @param[in] count Reserved entry count.
 */
Result capsaGetAlbumFileList(CapsAlbumStorage storage, u64 *out, CapsAlbumEntry *entries, u64 count);

/**
 * @brief Loads a file into the specified buffer.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[out] out_size Size of the AlbumFile.
 * @param[out] filebuf File output buffer.
 * @param[in] filebuf_size Size of the filebuf.
 */
Result capsaLoadAlbumFile(const CapsAlbumFileId *file_id, u64 *out_size, void* filebuf, u64 filebuf_size);

/**
 * @brief Deletes an AlbumFile corresponding to the specified \ref CapsAlbumFileId.
 * @param[in] file_id \ref CapsAlbumFileId
 */
Result capsaDeleteAlbumFile(const CapsAlbumFileId *file_id);

/**
 * @brief Copies an AlbumFile to the specified \ref CapsAlbumStorage.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] dst_storage \ref CapsAlbumStorage
 */
Result capsaStorageCopyAlbumFile(const CapsAlbumFileId *file_id, CapsAlbumStorage dst_storage);

/**
 * @brief Gets the mount status of the specified \ref CapsAlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] is_mounted Boolean over whether the storage is mounted or not.
 */
Result capsaIsAlbumMounted(CapsAlbumStorage storage, bool *is_mounted);

/**
 * @brief Returns the AlbumUsage for a specified \ref CapsAlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] out \ref CapsAlbumUsage2
 */
Result capsaGetAlbumUsage(CapsAlbumStorage storage, CapsAlbumUsage2 *out);

/**
 * @brief Gets the size for the specified AlbumFile.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[out] size Size of the file.
 */
Result capsaGetAlbumFileSize(const CapsAlbumFileId *file_id, u64 *size);

/**
 * @brief Load the Thumbnail for the specified AlbumFile.
 * @note Will always be 320x180.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[out] out_size Size of the Thumbnail.
 * @param[out] image JPEG image output buffer.
 * @param[in] image_size Image buffer size.
 */
Result capsaLoadAlbumFileThumbnail(const CapsAlbumFileId *file_id, u64 *out_size, void* image, u64 image_size);

/**
 * @brief Load the ScreenShotImage for the specified AlbumFile.
 * @note Only available on [2.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 1280x720.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotImage(u64 *width, u64 *height, const CapsAlbumFileId *file_id, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load the ScreenShotThumbnailImage for the specified AlbumFile.
 * @note Only available on [2.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 320x180.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotThumbnailImage(u64 *width, u64 *height, const CapsAlbumFileId *file_id, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load an \ref CapsAlbumEntry from a \ref CapsApplicationAlbumEntry and an ApplicationId.
 * @note Only available on [2.0.0+].
 * @param[out] entry \ref CapsAlbumEntry
 * @param[in] application_entry \ref CapsApplicationAlbumEntry
 * @param[in] application_id ApplicationId
 */
Result capsaGetAlbumEntryFromApplicationAlbumEntry(CapsAlbumEntry *entry, const CapsApplicationAlbumEntry *application_entry, u64 application_id);

/**
 * @brief Load the ScreenShotImage for the specified AlbumFile.
 * @note Only available on [3.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 1280x720.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotImageEx(u64 *width, u64 *height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load the ScreenShotThumbnailImage for the specified AlbumFile.
 * @note Only available on [3.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 320x180.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotThumbnailImageEx(u64 *width, u64 *height, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load the ScreenShotImage for the specified AlbumFile.
 * @note Only available on [3.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[out] attr \ref CapsScreenShotAttribute
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 1280x720.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotImageEx0(u64 *width, u64 *height, CapsScreenShotAttribute *attr, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Returns the AlbumUsage for a specified \ref CapsAlbumStorage.
 * @note Only available on [4.0.0+].
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] out \ref CapsAlbumUsage3
 */
Result capsaGetAlbumUsage3(CapsAlbumStorage storage, CapsAlbumUsage3 *out);

/**
 * @brief Returns the result for a AlbumStorage mount.
 * @note Only available on [4.0.0+].
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capsaGetAlbumMountResult(CapsAlbumStorage storage);

/**
 * @brief Returns the AlbumUsage for a specified \ref CapsAlbumStorage.
 * @note Only available on [4.0.0+].
 * @param[in] storage \ref CapsAlbumStorage
 * @param[in] flags \ref CapsAlbumFileContentsFlag
 * @param[out] out \ref CapsAlbumUsage16
 */
Result capsaGetAlbumUsage16(CapsAlbumStorage storage, u8 flags, CapsAlbumUsage16 *out);

/**
 * @brief Returns the start and end of the Applet Id range.
 * @note Only available on [6.0.0+].
 * @param[out] success Returns bool over whether the call was handled or not.
 * @param[out] min Mimimum applet id. Always 0x0100000000001000
 * @param[out] max Maximum applet id. Always 0x0100000000001FFF
 */
Result capsaGetMinMaxAppletId(bool* success, u64* min, u64* max);

/**
 * @brief Gets the amount of files of the specified type at a AlbumStorage.
 * @note Only available on [5.0.0+].
 * @param[in] storage \ref CapsAlbumStorage
 * @param[in] flags \ref CapsAlbumFileContentsFlag
 * @param[out] count Amount of files.
 */
Result capsaGetAlbumFileCountEx0(CapsAlbumStorage storage, u8 flags, u64 *count);

/**
 * @brief Gets a listing of \ref CapsAlbumEntry, where the AlbumFile's storage and type matches the input one.
 * @note Only available on [5.0.0+].
 * @param[in] storage \ref CapsAlbumStorage
 * @param[in] flags \ref CapsAlbumFileContentsFlag
 * @param[out] out Total output entries.
 * @param[out] entries Output array of \ref CapsAlbumEntry.
 * @param[in] count Reserved entry count.
 */
Result capsaGetAlbumFileListEx0(CapsAlbumStorage storage, u8 flags, u64 *out, CapsAlbumEntry *entries, u64 count);

/**
 * @brief Returns the image from the last shown ScreenShot Overlay.
 * @param[out] file_id \ref CapsAlbumFileId
 * @param[out] out_size Size of the thumbnail image. Always 0x5100.
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 96×54.
 */
Result capsaGetLastOverlayScreenShotThumbnail(CapsAlbumFileId *file_id, u64 *out_size, void* image, u64 image_size);

/**
 * @brief Returns the image from the last shown Movie Overlay.
 * @note Only available on [4.0.0+].
 * @param[out] file_id \ref CapsAlbumFileId
 * @param[out] out_size Size of the thumbnail image. Always 0x5100.
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 96×54.
 */
Result capsaGetLastOverlayMovieThumbnail(CapsAlbumFileId *file_id, u64 *out_size, void* image, u64 image_size);

/**
 * @brief Gets the currently set autosaving storage.
 * @note Wrapper around setsysGetPrimaryAlbumStorage but defaults to NAND if SD isn't available.
 * @param[out] storage \ref CapsAlbumStorage
 */
Result capsaGetAutoSavingStorage(CapsAlbumStorage *storage);

/**
 * @brief Gets required size to copy all files from one Storage to another.
 * @param[in] dst_storage \ref CapsAlbumStorage
 * @param[in] src_storage \ref CapsAlbumStorage
 * @param[out] out Required storage space size.
 */
Result capsaGetRequiredStorageSpaceSizeToCopyAll(CapsAlbumStorage dst_storage, CapsAlbumStorage src_storage, u64 *out);

/**
 * @brief Load the ScreenShotThumbnailImage for the specified AlbumFile.
 * @note Only available on [3.0.0+].
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[out] attr \ref CapsScreenShotAttribute
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 320x180.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsLoadAlbumScreenShotThumbnailImageEx0(u64 *width, u64 *height, CapsScreenShotAttribute *attr, const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load the ScreenShotImage for the specified AlbumFile.
 * @note Only available on [4.0.0+].
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] out \ref CapsLoadAlbumScreenShotImageOutput
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 1280x720.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotImageEx1(const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, CapsLoadAlbumScreenShotImageOutput *out, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Load the ScreenShotThumbnailImage for the specified AlbumFile.
 * @note Only available on [4.0.0+].
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] opts \ref CapsScreenShotDecodeOption
 * @param[out] out \ref CapsLoadAlbumScreenShotImageOutput
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Image buffer size, should be at least large enough for RGBA8 320x180.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[in] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 */
Result capsaLoadAlbumScreenShotThumbnailImageEx1(const CapsAlbumFileId *file_id, const CapsScreenShotDecodeOption *opts, CapsLoadAlbumScreenShotImageOutput *out, void* image, u64 image_size, void* workbuf, u64 workbuf_size);

/**
 * @brief Unmounts the specified AlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capsaForceAlbumUnmounted(CapsAlbumStorage storage);

/**
 * @brief Resets mount status for the specified AlbumStorage.
 * @note Mounts the Storage if available.
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capsaResetAlbumMountStatus(CapsAlbumStorage storage);

/**
 * @brief Refreshs Album Cache for the specified AlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capsaRefreshAlbumCache(CapsAlbumStorage storage);

/**
 * @brief Gets the AlbumCache of the specified AlbumStorage.
 * @note Stubbed on [4.0.0+].
 * @note use \ref capsaGetAlbumCacheEx instead.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[out] cache \ref CapsAlbumCache
 */
Result capsaGetAlbumCache(CapsAlbumStorage storage, CapsAlbumCache *cache);

/**
 * @brief Gets the AlbumCache for the specified type of the specified AlbumStorage.
 * @param[in] storage \ref CapsAlbumStorage
 * @param[in] contents \ref CapsAlbumFileContents
 * @param[out] cache \ref CapsAlbumCache
 */
Result capsaGetAlbumCacheEx(CapsAlbumStorage storage, CapsAlbumFileContents contents, CapsAlbumCache *cache);

/**
 * @brief Load an \ref CapsAlbumEntry from a \ref CapsApplicationAlbumEntry and an AppletResourceUserId.
 * @note Only available on [2.0.0+].
 * @param[out] entry \ref CapsAlbumEntry
 * @param[in] application_entry \ref CapsApplicationAlbumEntry
 */
Result capsaGetAlbumEntryFromApplicationAlbumEntryAruid(CapsAlbumEntry *entry, const CapsApplicationAlbumEntry *application_entry);

/**
 * @brief Opens an AlbumMovieStream.
 * @note This opens IAlbumAccessorSession if not previously opened, it's closed during \ref capsaExit.
 * @note Up to 4 streams can be open at the same time. Multiple streams can be open at the same time for the same \ref CapsAlbumFileId.
 * @note Only available on [4.0.0+].
 * @param[out] stream Stream handle.
 * @param[in] entry \ref CapsAlbumFileId
 */
Result capsaOpenAlbumMovieStream(u64 *stream, const CapsAlbumFileId *file_id);

/**
 * @brief Closes an AlbumMovieStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 */
Result capsaCloseAlbumMovieStream(u64 stream);

/**
 * @brief Gets the data size of an AlbumMovieStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] size Size of the actual MP4, without the JPEG at the end.
 */
Result capsaGetAlbumMovieStreamSize(u64 stream, u64 *size);

/**
 * @brief Reads data from an AlbumMovieStream.
 * @note offset(+size) must not be negative. offset and size must be aligned to 0x40000-bytes.
 * @note When offset(+size) goes beyond the size from \ref capsaGetAlbumMovieStreamSize, the regions of the buffer which goes beyond that are cleared to 0, and actual_size is still set to the input size.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[in] offset Offset.
 * @param[out] Output data buffer.
 * @param[in] size Data buffer size.
 * @param[out] actual_size Actual read size.
 */
Result capsaReadMovieDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size);

/**
 * @brief Gets the BrokenReason for an AlbumMovieStream.
 * @note Official sw doesn't use this.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 */
Result capsaGetAlbumMovieReadStreamBrokenReason(u64 stream);

/**
 * @brief Gets the data size of an Image taken from an AlbumMovieStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] size Expected size of an Image.
 */
Result capsaGetAlbumMovieReadStreamImageDataSize(u64 stream, u64 *size);

/**
 * @brief Reads data of an Image taken from an AlbumMovieStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[in] offset Offset.
 * @param[out] Output data buffer.
 * @param[in] size Data buffer size.
 * @param[out] actual_size Actual read size.
 */
Result capsaReadImageDataFromAlbumMovieReadStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size);

/**
 * @brief Gets the file attribute of an AlbumMovieStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] attr \ref CapsScreenShotAttribute
 */
Result capsaReadFileAttributeFromAlbumMovieReadStream(u64 stream, CapsScreenShotAttribute *attr);
