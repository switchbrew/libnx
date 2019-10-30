/**
 * @file capssu.h
 * @brief Application screenshot saving (caps:su) service IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/acc.h"
#include "../services/caps.h"

/// Initialize caps:su. Only available on [4.0.0+].
Result capssuInitialize(void);

/// Exit caps:su.
void capssuExit(void);

/// Gets the Service for caps:su.
Service* capssuGetServiceSession(void);

/**
 * @brief This is a wrapper for \ref capssuSaveScreenShotEx0.
 * @note This uses an all-zero \ref CapsScreenShotAttribute with orientation = input orientation, and unk_xc = 1.
 * @param[in] buffer RGBA8 1280x720 image buffer.
 * @param[in] size Size of the buffer.
 * @param[in] reportoption \ref AlbumReportOption
 * @param[in] orientation \ref AlbumImageOrientation
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShot(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, CapsApplicationAlbumEntry *out);

/**
 * @brief Similar to \ref capssuSaveScreenShot, except this is a wrapper for \ref capssuSaveScreenShotEx1.
 * @note This uses an all-zero \ref CapsScreenShotAttribute with orientation = input orientation, and unk_xc = 1.
 * @note Only available on [8.0.0+].
 * @param[in] buffer RGBA8 1280x720 image data buffer.
 * @param[in] size Size of the buffer.
 * @param[in] reportoption \ref AlbumReportOption
 * @param[in] orientation \ref AlbumImageOrientation
 * @param[in] userdata Input UserData buffer. If NULL, the \ref CapsApplicationData will be empty.
 * @param[in] userdata_size Input UserData size, must be within bounds for CapsApplicationData::userdata. If 0, the \ref CapsApplicationData will be empty.
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShotWithUserData(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, const void* userdata, size_t userdata_size, CapsApplicationAlbumEntry *out);

/**
 * @brief Similar to \ref capssuSaveScreenShot, except this is a wrapper for \ref capssuSaveScreenShotEx2.
 * @note This uses an all-zero \ref CapsScreenShotAttribute with orientation = input orientation, and unk_xc = 1.
 * @note Only available on [6.0.0+].
 * @param[in] buffer RGBA8 1280x720 image data buffer.
 * @param[in] size Size of the buffer.
 * @param[in] reportoption \ref AlbumReportOption
 * @param[in] orientation \ref AlbumImageOrientation
 * @param[in] uids Input array of \ref AccountUid. If NULL, the \ref CapsUserIdList will be empty.
 * @param[in] uid_count Size of the uids array in entries, must be within bounds for CapsUserIdList::uids. If 0, the \ref CapsUserIdList will be empty.
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShotWithUserIds(const void* buffer, size_t size, AlbumReportOption reportoption, AlbumImageOrientation orientation, const AccountUid* uids, size_t uid_count, CapsApplicationAlbumEntry *out);

/**
 * @brief Saves an Album screenshot using the specified gfx data in the buffer, with the specified \ref CapsScreenShotAttribute.
 * @param[in] buffer RGBA8 1280x720 image data buffer.
 * @param[in] size Size of the buffer, must be at least 0x384000.
 * @param[in] attr \ref CapsScreenShotAttribute
 * @param[in] reportoption \ref AlbumReportOption
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShotEx0(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationAlbumEntry *out);

/**
 * @brief Same as \ref capssuSaveScreenShotEx0, except this allows specifying the \ref CapsApplicationData.
 * @note Only available on [8.0.0+].
 * @param[in] buffer RGBA8 1280x720 image data buffer.
 * @param[in] size Size of the buffer, must be at least 0x384000.
 * @param[in] attr \ref CapsScreenShotAttribute
 * @param[in] reportoption \ref AlbumReportOption
 * @param[in] appdata \ref CapsApplicationData
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShotEx1(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsApplicationData *appdata, CapsApplicationAlbumEntry *out);

/**
 * @brief Same as \ref capssuSaveScreenShotEx0, except this allows specifying the \ref CapsUserIdList.
 * @note Only available on [6.0.0+].
 * @param[in] buffer RGBA8 1280x720 image data buffer.
 * @param[in] size Size of the buffer, must be at least 0x384000.
 * @param[in] attr \ref CapsScreenShotAttribute
 * @param[in] reportoption \ref AlbumReportOption
 * @param[in] list \ref CapsUserIdList
 * @param[out] out \ref CapsApplicationAlbumEntry. Optional, can be NULL.
 */
Result capssuSaveScreenShotEx2(const void* buffer, size_t size, const CapsScreenShotAttribute *attr, AlbumReportOption reportoption, CapsUserIdList *list, CapsApplicationAlbumEntry *out);

