/**
 * @file capsdc.h
 * @brief Jpeg Decoder (caps:dc) service IPC wrapper. Only available on [4.0.0+].
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
 * @param[in] opts \ref CapsScreenShotDecodeOption.
 * @param[in] jpeg Jpeg image input buffer.
 * @param[in] jpeg_size Input image buffer size.
 * @param[out] out_image RGBA8 image output buffer.
 * @param[in] out_image_size Output image buffer size, should be at least large enough for RGBA8 width x height.
 */
Result capsdcDecodeJpeg(u32 width, u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_image, size_t out_image_size);
