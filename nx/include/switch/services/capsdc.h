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

/**
 * @brief Shrinks a jpeg's dimensions by 2.
 * @note Tries to compress with jpeg quality in this order: 98, 95, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0.
 * @note Only available on [17.0.0+].
 * @param[in] width Input image width.
 * @param[in] height Input image width.
 * @param[in] opts \ref CapsScreenShotDecodeOption.
 * @param[in] jpeg Jpeg image input buffer.
 * @param[in] jpeg_size Input image buffer size.
 * @param[out] out_jpeg Jpeg image output buffer
 * @param[in] out_jpeg_size Output image buffer size.
 * @param[out] out_result_size size of the resulting JPEG.
 */
Result capsdcShrinkJpeg(u32 width, u32 height, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_jpeg, size_t out_jpeg_size, u64 *out_result_size);

/**
 * @brief Shrinks a jpeg.
 * @note Fails if the scaled size is larger than the original or the output buffer isn't large enough.
 * @note Only available on [19.0.0+].
 * @param[in] scaled_width Wanted image width.
 * @param[in] scaled_height Wanted image width.
 * @param[in] jpeg_quality has to be in range 0-100.
 * @param[in] opts \ref CapsScreenShotDecodeOption.
 * @param[in] jpeg Jpeg image input buffer.
 * @param[in] jpeg_size Input image buffer size.
 * @param[out] out_jpeg Jpeg image output buffer
 * @param[in] out_jpeg_size Output image buffer size.
 * @param[out] out_result_size size of the resulting jpeg.
 */
Result capsdcShrinkJpegEx(u32 scaled_width, u32 scaled_height, u32 jpeg_quality, const CapsScreenShotDecodeOption *opts, const void* jpeg, size_t jpeg_size, void* out_jpeg, size_t out_jpeg_size, u64 *out_result_size);
