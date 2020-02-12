/**
 * @file capsc.h
 * @brief Album Control (caps:c) service IPC wrapper.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"

/// Initialize caps:c
Result capscInitialize(void);

/// Exit caps:c.
void capscExit(void);

/// Gets the Service for caps:c.
Service* capscGetServiceSession(void);

/**
 * @brief Notify the service that a storage is now available.
 * @note This will result in capsrv mounting the image directory on that storage medium.
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capscNotifyAlbumStorageIsAvailable(CapsAlbumStorage storage);

/**
 * @brief Notify the service that a storage is now unavailable.
 * @note This will result in capsrv unmounting the image directory on that storage medium.
 * @param[in] storage \ref CapsAlbumStorage
 */
Result capscNotifyAlbumStorageIsUnAvailable(CapsAlbumStorage storage);

/**
 * @brief Register an applet for later usage.
 * @note Called at application launch.
 * @note Will generate a random AES-256 key for this application for use on Shim-Version 0.
 * @param[in] appletResourceUserId Session unique applet identifier.
 * @param[in] application_id Title unique identifier.
 */
Result capscRegisterAppletResourceUserId(u64 appletResourceUserId, u64 application_id);

/**
 * @brief Unregister an applet.
 * @note Called at application exit.
 * @param[in] appletResourceUserId Session unique applet identifier.
 * @param[in] application_id Title unique identifier.
 */
Result capscUnregisterAppletResourceUserId(u64 appletResourceUserId, u64 application_id);

/**
 * @brief Get an Application ID that corresponse to an Application Resource User ID.
 * @note Returns value set by \ref capscRegisterAppletResourceUserId.
 * @param[out] application_id Title unique identifier.
 * @param[in] appletResourceUserId Session unique applet identifier.
 */
Result capscGetApplicationIdFromAruid(u64 *application_id, u64 aruid);

/**
 * @brief Checks whether an Application ID is registered.
 * @param[in] application_id Title unique identifier.
 */
Result capscCheckApplicationIdRegistered(u64 application_id);

/**
 * @brief Generate an Album File ID based of parameters and current time.
 * @param[in] application_id Title unique identifier.
 * @param[in] contents \ref CapsAlbumFileContents
 * @param[out] file_id \ref CapsAlbumFileId
 */
Result capscGenerateCurrentAlbumFileId(u64 application_id, CapsAlbumFileContents contents, CapsAlbumFileId *file_id);

/**
 * @brief Generate an Application Album Entry based of parameters.
 * @note Output will be different between Shim Version 0 and 1.
 * @param[out] appEntry \ref CapsApplicationAlbumEntry
 * @param[in] entry \ref CapsAlbumEntry
 * @param[in] application_id Title unique identifier.
 */
Result capscGenerateApplicationAlbumEntry(CapsApplicationAlbumEntry *appEntry, const CapsAlbumEntry *entry, u64 application_id);

/**
 * @brief Save a jpeg image.
 * @note Only available on [2.0.0-3.0.2].
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] buffer JPEG image buffer.
 * @param[in] buffer_size Size of the JPEG image.
 */
Result capscSaveAlbumScreenShotFile(const CapsAlbumFileId *file_id, const void* buffer, u64 buffer_size);

/**
 * @brief Save a jpeg image.
 * @note Only available on [4.0.0+].
 * @note Version 3 as of [9.1.0].
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] version Revision number.
 * @param[in] makernote_offset Offset to makernote in JPEG buffer.
 * @param[in] makernote_size Size of the makernote in JPEG buffer.
 * @param[in] buffer JPEG image buffer.
 * @param[in] buffer_size Size of the JPEG image.
 */
Result capscSaveAlbumScreenShotFileEx(const CapsAlbumFileId *file_id, u64 version, u64 makernote_offset, u64 makernote_size, const void* buffer, u64 buffer_size);

/**
 * @brief Sets thumbnail data for the last taken screenshot.
 * @note 96×54x4 Image will get saved.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] image RGBA8 image buffer.
 * @param[in] image_size size of the RGBA8 image buffer.
 */
Result capscSetOverlayScreenShotThumbnailData(const CapsAlbumFileId *file_id, const void* image, u64 image_size);

/**
 * @brief Sets thumbnail data for the last recorded movie.
 * @note Only availabe on [4.0.0+].
 * @note 96×54x4 Image will get saved.
 * @param[in] file_id \ref CapsAlbumFileId
 * @param[in] image RGBA8 image buffer.
 * @param[in] image_size size of the RGBA8 image buffer.
 */
Result capscSetOverlayMovieThumbnailData(const CapsAlbumFileId *file_id, const void* image, u64 image_size);

/**
 * @brief Opens an AlbumMovieReadStream.
 * @note This opens IAlbumControlSession if not previously opened, it's closed during \ref capsaExit.
 * @note Up to 4 streams can be open at the same time. Multiple streams can be open at the same time for the same \ref CapsAlbumFileId.
 * @note Only available on [4.0.0+].
 * @param[out] stream Stream handle.
 * @param[in] entry \ref CapsAlbumFileId
 */
Result capscOpenAlbumMovieReadStream(u64 *stream, const CapsAlbumFileId *file_id);

/**
 * @brief Closes an AlbumMovieReadStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 */
Result capscCloseAlbumMovieStream(u64 stream);

/**
 * @brief Gets the data size of an AlbumMovieReadStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] size Size of the actual MP4, without the JPEG at the end.
 */
Result capscGetAlbumMovieStreamSize(u64 stream, u64 *size);

/**
 * @brief Reads data from an AlbumMovieReadStream.
 * @note offset(+size) must not be negative. offset and size must be aligned to 0x40000-bytes.
 * @note When offset(+size) goes beyond the size from \ref capsaGetAlbumMovieStreamSize, the regions of the buffer which goes beyond that are cleared to 0, and actual_size is still set to the input size.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[in] offset Offset.
 * @param[out] Output data buffer.
 * @param[in] size Data buffer size.
 * @param[out] actual_size Actual read size.
 */
Result capscReadMovieDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size);

/**
 * @brief Gets the BrokenReason for an AlbumMovieReadStream.
 * @note Official sw doesn't use this.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 */
Result capscGetAlbumMovieReadStreamBrokenReason(u64 stream);

/**
 * @brief Gets the data size of an Image taken from an AlbumMovieReadStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] size Expected size of an Image.
 */
Result capscGetAlbumMovieReadStreamImageDataSize(u64 stream, u64 *size);

/**
 * @brief Reads data of an Image taken from an AlbumMovieReadStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[in] offset Offset.
 * @param[out] Output data buffer.
 * @param[in] size Data buffer size.
 * @param[out] actual_size Actual read size.
 */
Result capscReadImageDataFromAlbumMovieReadStream(u64 stream, u64 offset, void* buffer, size_t size, u64 *actual_size);

/**
 * @brief Gets the file attribute of an AlbumMovieReadStream.
 * @note Only available on [4.0.0+].
 * @param[in] stream Stream handle.
 * @param[out] attr \ref CapsScreenShotAttribute
 */
Result capscReadFileAttributeFromAlbumMovieReadStream(u64 stream, CapsScreenShotAttribute *attribute);

/**
 * @brief Opens an AlbumMovieWriteStream.
 * @note This opens IAlbumControlSession if not previously opened, it's closed during \ref capsaExit.
 * @note Up to 2 streams can be open at the same time.
 * @note Only available on [4.0.0+].
 * @param[out] stream Stream handle.
 * @param[in] entry \ref CapsAlbumFileId
 */
Result capscOpenAlbumMovieWriteStream(u64 *stream, const CapsAlbumFileId *file_id);
Result capscFinishAlbumMovieWriteStream(u64 stream);
Result capscCommitAlbumMovieWriteStream(u64 stream);
Result capscDiscardAlbumMovieWriteStream(u64 stream);
Result capscDiscardAlbumMovieWriteStreamNoDelete(u64 stream);
Result capscCommitAlbumMovieWriteStreamEx(u64 stream, CapsAlbumCommitOutput *out);
Result capscStartAlbumMovieWriteStreamDataSection(u64 stream);
Result capscEndAlbumMovieWriteStreamDataSection(u64 stream);
Result capscStartAlbumMovieWriteStreamMetaSection(u64 stream);
Result capscEndAlbumMovieWriteStreamMetaSection(u64 stream);
Result capscReadDataFromAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size, u64 *actual_size);
Result capscWriteDataToAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size);
Result capscWriteMetaToAlbumMovieWriteStream(u64 stream, u64 offset, void* buffer, u64 size);
Result capscGetAlbumMovieWriteStreamBrokenReason(u64 stream);
Result capscGetAlbumMovieWriteStreamDataSize(u64 stream, u64 *size);
Result capscSetAlbumMovieWriteStreamDataSize(u64 stream, u64 size);
