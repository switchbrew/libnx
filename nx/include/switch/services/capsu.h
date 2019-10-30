/**
 * @file capsu.h
 * @brief Application Album (caps:u) service IPC wrapper.
 * This is only usable with AlbumFiles associated with the current host Application.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"
#include "../services/acc.h"

/// Initialize caps:u. Only available on [5.0.0+].
Result capsuInitialize(void);

/// Exit caps:u.
void capsuExit(void);

/// Gets the Service for caps:u.
Service* capsuGetServiceSession(void);

/// Gets the Service for IAlbumAccessorApplicationSession, only initialized after \ref capsuOpenAlbumMovieStream was used (unaffected by using \ref capsuCloseAlbumMovieStream).
Service* capsuGetServiceSession_Accessor(void);

/**
 * @brief Gets a listing of \ref CapsApplicationAlbumFileEntry.
 * @note On [6.0.0+] this uses GetAlbumFileList1AafeAruidDeprecated, otherwise this uses GetAlbumFileList0AafeAruidDeprecated.
 * @note This is an old version of \ref capsuGetAlbumFileList3.
 * @param[out] entries Output array of \ref CapsApplicationAlbumFileEntry.
 * @param[in] count Max size of the output array in entries.
 * @param[in] type \ref CapsContentType
 * @param[in] start_datetime Start \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] end_datetime End \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[out] total_entries Total output entries.
 */
Result capsuGetAlbumFileListDeprecated1(CapsApplicationAlbumFileEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, s32 *total_entries);

/**
 * @brief Gets a listing of \ref CapsApplicationAlbumFileEntry, where the AlbumFile has an UserId which matches the input one. See also \ref capssuSaveScreenShotWithUserIds.
 * @note Only available on [6.0.0+].
 * @note This is an old version of \ref capsuGetAlbumFileList4.
 * @param[out] entries Output array of \ref CapsApplicationAlbumFileEntry.
 * @param[in] count Max size of the output array in entries.
 * @param[in] type \ref CapsContentType
 * @param[in] start_datetime Start \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] end_datetime End \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] uid \ref AccountUid
 * @param[out] total_entries Total output entries.
 */
Result capsuGetAlbumFileListDeprecated2(CapsApplicationAlbumFileEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, AccountUid uid, s32 *total_entries);

/**
 * @brief Gets a listing of \ref CapsApplicationAlbumEntry.
 * @note Only available on [7.0.0+], on prior sysvers use \ref capsuGetAlbumFileListDeprecated1 instead.
 * @param[out] entries Output array of \ref CapsApplicationAlbumEntry.
 * @param[in] count Max size of the output array in entries.
 * @param[in] type \ref CapsContentType
 * @param[in] start_datetime Start \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] end_datetime End \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[out] total_entries Total output entries.
 */
Result capsuGetAlbumFileList3(CapsApplicationAlbumEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, s32 *total_entries);

/**
 * @brief Gets a listing of \ref CapsApplicationAlbumEntry, where the AlbumFile has an UserId which matches the input one. See also \ref capssuSaveScreenShotWithUserIds.
 * @note Only available on [7.0.0+], on prior sysvers use \ref capsuGetAlbumFileListDeprecated2 instead.
 * @param[out] entries Output array of \ref CapsApplicationAlbumEntry.
 * @param[in] count Max size of the output array in entries.
 * @param[in] type \ref CapsContentType
 * @param[in] start_datetime Start \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] end_datetime End \ref CapsAlbumFileDateTime, when NULL the default is used.
 * @param[in] uid \ref AccountUid
 * @param[out] total_entries Total output entries.
 */
Result capsuGetAlbumFileList4(CapsApplicationAlbumEntry *entries, s32 count, CapsContentType type, const CapsAlbumFileDateTime *start_datetime, const CapsAlbumFileDateTime *end_datetime, AccountUid uid, s32 *total_entries);

/**
 * @brief Deletes the specified AlbumFile.
 * @param[in] type \ref CapsContentType, must match ::CapsContentType_ExtraMovie.
 * @param[in] entry \ref CapsApplicationAlbumFileEntry
 */
Result capsuDeleteAlbumFile(CapsContentType type, const CapsApplicationAlbumFileEntry *entry);

/**
 * @brief Gets the filesize for the entire specified AlbumFile.
 * @param[in] entry \ref CapsApplicationAlbumFileEntry
 * @param[out] size Output filesize.
 */
Result capsuGetAlbumFileSize(const CapsApplicationAlbumFileEntry *entry, u64 *size);

/**
 * @brief Load the ScreenShotImage for the specified AlbumFile.
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[out] attr \ref CapsScreenShotAttributeForApplication
 * @param[out] userdata Output buffer containing the UserData. Optional, can be NULL. This buffer is cleared to 0 using userdata_maxsize, prior to doing the memcpy.
 * @param[in] userdata_maxsize Max size of the userdata buffer. Optional, can be 0.
 * @param[out] userdata_size Userdata size field, clamped to max size sizeof(CapsApplicationData::userdata) when needed.
 * @param[out] image RGBA8 image output buffer.
 * @param[out] image_size Image buffer size, should be at least large enough for RGBA8 1280x720.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[out] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 * @param[in] entry \ref CapsApplicationAlbumFileEntry
 * @param[in] option \ref CapsScreenShotDecodeOption
 */
Result capsuLoadAlbumScreenShotImage(s32 *width, s32 *height, CapsScreenShotAttributeForApplication *attr, void* userdata, size_t userdata_maxsize, u32 *userdata_size, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option);

/**
 * @brief Load the ScreenShotThumbnailImage for the specified AlbumFile.
 * @param[out] width Output image width. Optional, can be NULL.
 * @param[out] height Output image height. Optional, can be NULL.
 * @param[out] attr \ref CapsScreenShotAttributeForApplication
 * @param[out] userdata Output buffer containing the UserData. Optional, can be NULL. This buffer is cleared to 0 using userdata_maxsize, prior to doing the memcpy.
 * @param[in] userdata_maxsize Max size of the userdata buffer. Optional, can be 0.
 * @param[out] userdata_size Userdata size field, clamped to max size sizeof(CapsApplicationData::userdata) when needed.
 * @param[out] image RGBA8 image output buffer.
 * @param[out] image_size Image buffer size, should be at least large enough for RGBA8 320x180.
 * @param[out] workbuf Work buffer, cleared to 0 by the cmd before it returns.
 * @param[out] workbuf_size Work buffer size, must be at least the size of the JPEG within the AlbumFile.
 * @param[in] entry \ref CapsApplicationAlbumFileEntry
 * @param[in] option \ref CapsScreenShotDecodeOption
 */
Result capsuLoadAlbumScreenShotThumbnailImage(s32 *width, s32 *height, CapsScreenShotAttributeForApplication *attr, void* userdata, size_t userdata_maxsize, u32 *userdata_size, void* image, size_t image_size, void* workbuf, size_t workbuf_size, const CapsApplicationAlbumFileEntry *entry, const CapsScreenShotDecodeOption *option);

/**
 * @brief PrecheckToCreateContents. Official sw only uses this with ::CapsContentType_ExtraMovie.
 * @param[in] type \ref CapsContentType
 * @param[in] unk Unknown.
 */
Result capsuPrecheckToCreateContents(CapsContentType type, u64 unk);

/**
 * @brief Opens an AlbumMovieStream.
 * @note This opens IAlbumAccessorApplicationSession if not previously opened, it's closed during \ref capsuExit.
 * @note Up to 4 streams can be open at the same time. Multiple streams can be open at the same time for the same \ref CapsApplicationAlbumFileEntry.
 * @param[out] stream Stream handle.
 * @param[in] entry \ref CapsApplicationAlbumFileEntry
 */
Result capsuOpenAlbumMovieStream(u64 *stream, const CapsApplicationAlbumFileEntry *entry);

/**
 * @brief Closes an AlbumMovieStream.
 * @param[in] stream Stream handle.
 */
Result capsuCloseAlbumMovieStream(u64 stream);

/**
 * @brief Gets the data size of an AlbumMovieStream.
 * @param[in] stream Stream handle.
 * @param[out] size Size of the actual MP4, without the JPEG at the end.
 */
Result capsuGetAlbumMovieStreamSize(u64 stream, u64 *size);

/**
 * @brief Reads data from an AlbumMovieStream.
 * @note offset(+size) must not be negative. offset and size must be aligned to 0x40000-bytes.
 * @note When offset(+size) goes beyond the size from \ref capsuGetAlbumMovieStreamSize, the regions of the buffer which goes beyond that are cleared to 0, and actual_size is still set to the input size.
 * @param[in] stream Stream handle.
 * @param[in] offset Offset.
 * @param[out] Output data buffer.
 * @param[in] size Data buffer size.
 * @param[out] actual_size Actual read size.
 */
Result capsuReadAlbumMovieStream(u64 stream, s64 offset, void* buffer, size_t size, u64 *actual_size);

/**
 * @brief Gets the BrokenReason for an AlbumMovieStream.
 * @note Official sw doesn't use this.
 * @param[in] stream Stream handle.
 */
Result capsuGetAlbumMovieStreamBrokenReason(u64 stream);

