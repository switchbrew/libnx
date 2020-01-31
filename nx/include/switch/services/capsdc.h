/**
 * @file capsdc.h
 * @brief Jpeg Decoder (caps:dc) service IPC wrapper. Only Available on [4.0.0+].
 * @note Only holds one session that is occupied by capsrv.
 * @author Behemoth
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../sf/service.h"
#include "../services/caps.h"

/// Initialize caps:dc
Result capsdcInitialize(void);

/// Exit caps:dc.
void capsdcExit(void);

/// Gets the Service for caps:dc.
Service* capsdcGetServiceSession(void);

/**
 * @brief Decodes a jpeg buffer into RGBX.
 * @param[in] width Image width.
 * @param[in] height Image height.
 * @param[in] opts CapsScreenShotDecodeOption decode options.
 * @param[in] jpeg Jpeg image input buffer.
 * @param[in] jpeg_size Input image buffer size.
 * @param[out] image RGBA8 image output buffer.
 * @param[in] image_size Output image buffer size, should be at least large enough for RGBA8 width x height.
 */
Result capsdcDecodeJpeg(u32 width, u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, const u64 jpeg_size, void* image, const u64 image_size);
