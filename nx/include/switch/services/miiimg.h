/**
 * @file miiimg.h
 * @brief Mii image (miiimg) service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/mii.h"

/// Image ID.
typedef struct {
    Uuid uuid;
} MiiimgImageId;

/// Image attribute.
typedef struct {
    MiiimgImageId image_id;    ///< Image ID.
    MiiCreateId create_id;     ///< Mii's create ID.
    u32 unk;
    u16 mii_name[10+1];        ///< utf-16be, null-terminated
} NX_PACKED MiiimgImageAttribute;

/// Initialize miiimg.
Result miiimgInitialize(void);

/// Exit miiimg.
void miiimgExit(void);

/// Gets the Service object for the actual miiimg service session.
Service* miiimgGetServiceSession(void);

/**
 * @brief Reloads the image database.
 */
Result miiimgReload(void);

/**
 * @brief Gets the number of mii images in the database.
 * @param[out] out_count Mii image count.
 */
Result miiimgGetCount(s32 *out_count);

/**
 * @brief Gets whether the image database is empty.
 * @param[out] out_empty Whether the database is empty.
 */
Result miiimgIsEmpty(bool *out_empty);

/**
 * @brief Gets whether the image database is full.
 * @param[out] out_empty Whether the database is full.
 */
Result miiimgIsFull(bool *out_full);

/**
 * @brief Gets the image attribute for the specified image index.
 * @param[in] index Image index.
 * @param[out] out_attr Out image attribute.
 */
Result miiimgGetAttribute(s32 index, MiiimgImageAttribute *out_attr);

/**
 * @brief Loads the image data (raw RGBA8) for the specified image ID.
 * @note Server doesn't seem to check the image buffer size, but 0x40000 is the optimal size.
 * @param[in] id Input image ID.
 * @param[out] out_image Out iamge buffer.
 * @param[in] out_image_size Out image buffer size.
 */
Result miiimgLoadImage(MiiimgImageId id, void* out_image, size_t out_image_size);
