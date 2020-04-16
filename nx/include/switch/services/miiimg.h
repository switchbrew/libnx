/**
 * @file miiimg.h
 * @brief Mii image (miiimg) service IPC wrapper.
 * @author XorTroll
 * @copyright libnx Authors
 */
#pragma once
#include "mii.h"

typedef struct {
    Uuid uuid;
} MiiimgImageId;

typedef struct {
    MiiimgImageId image_id;
    MiiCreateId create_id;
    u32 unk;
    u16 mii_name[10+1];
} PACKED MiiimgImageAttribute;

/// Initialize miiimg.
Result miiimgInitialize(void);

/// Exit miiimg.
void miiimgExit(void);

/// Gets the Service object for the actual miiimg service session.
Service* miiimgGetServiceSession(void);

Result miiimgReload();
Result miiimgGetCount(u32 *out_count);
Result miiimgIsEmpty(u8 *out_empty);
Result miiimgIsFull(u8 *out_full);
Result miiimgGetAttribute(u32 index, MiiimgImageAttribute *out_attr);
Result miiimgLoadImage(MiiimgImageId id, void *out_image, size_t out_image_size);
