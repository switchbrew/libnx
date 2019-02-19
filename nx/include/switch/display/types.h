/**
 * @file display/types.h
 * @brief Definitions for Android-related types and enumerations.
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

// From Android PixelFormat.h & graphics-base-v1.0.h.
enum {
    PIXEL_FORMAT_RGBA_8888              = 1U,
    PIXEL_FORMAT_RGBX_8888              = 2U,
    PIXEL_FORMAT_RGB_888                = 3U,
    PIXEL_FORMAT_RGB_565                = 4U,
    PIXEL_FORMAT_BGRA_8888              = 5U,
    PIXEL_FORMAT_RGBA_5551              = 6U,
    PIXEL_FORMAT_RGBA_4444              = 7U,
    PIXEL_FORMAT_YCRCB_420_SP           = 17U,
    PIXEL_FORMAT_RAW16                  = 32U,
    PIXEL_FORMAT_BLOB                   = 33U,
    PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 34U,
    PIXEL_FORMAT_YCBCR_420_888          = 35U,
    PIXEL_FORMAT_Y8                     = 0x20203859U,
    PIXEL_FORMAT_Y16                    = 0x20363159U,
    PIXEL_FORMAT_YV12                   = 0x32315659U,
};

// From Android gralloc.h.
enum {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000U,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002U,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003U,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK          = 0x0000000FU,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000U,
    /* buffer is rarely written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020U,
    /* buffer is often written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030U,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0U,
    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE            = 0x00000100U,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER             = 0x00000200U,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D                 = 0x00000400U,
    /* buffer will be used by the HWComposer HAL module */
    GRALLOC_USAGE_HW_COMPOSER           = 0x00000800U,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB                 = 0x00001000U,
    /* buffer should be displayed full-screen on an external display when
     * possible */
    GRALLOC_USAGE_EXTERNAL_DISP         = 0x00002000U,
    /* Must have a hardware-protected path to external display sink for
     * this buffer.  If a hardware-protected path is not available, then
     * either don't composite only this buffer (preferred) to the
     * external sink, or (less desirable) do not route the entire
     * composition to the external sink.  */
    GRALLOC_USAGE_PROTECTED             = 0x00004000U,
    /* buffer may be used as a cursor */
    GRALLOC_USAGE_CURSOR                = 0x00008000U,
    /* buffer will be used with the HW video encoder */
    GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000U,
    /* buffer will be written by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000U,
    /* buffer will be read by the HW camera pipeline */
    GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000U,
    /* buffer will be used as part of zero-shutter-lag queue */
    GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000U,
    /* mask for the camera access values */
    GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000U,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK               = 0x00071F00U,
    /* buffer will be used as a RenderScript Allocation */
    GRALLOC_USAGE_RENDERSCRIPT          = 0x00100000U,
};

// From Android window.h.
/* attributes queriable with query() */
enum {
    NATIVE_WINDOW_WIDTH     = 0,
    NATIVE_WINDOW_HEIGHT    = 1,
    NATIVE_WINDOW_FORMAT    = 2,
//...
//    NATIVE_WINDOW_DEFAULT_WIDTH = 6, //These two return invalid data.
//    NATIVE_WINDOW_DEFAULT_HEIGHT = 7,
};

// From Android window.h.
/* parameter for NATIVE_WINDOW_[API_][DIS]CONNECT */
enum {
    //...
    /* Buffers will be queued after being filled using the CPU
     */
    NATIVE_WINDOW_API_CPU = 2,
    //...
};

// From Android hardware.h.

/**
 * Transformation definitions
 *
 * IMPORTANT NOTE:
 * HAL_TRANSFORM_ROT_90 is applied CLOCKWISE and AFTER HAL_TRANSFORM_FLIP_{H|V}.
 *
 */

enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
};

// From Android window.h.
/* parameter for NATIVE_WINDOW_SET_BUFFERS_TRANSFORM */
enum {
    /* flip source image horizontally */
    NATIVE_WINDOW_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    /* flip source image vertically */
    NATIVE_WINDOW_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    /* rotate source image 90 degrees clock-wise */
    NATIVE_WINDOW_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    /* rotate source image 180 degrees */
    NATIVE_WINDOW_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    /* rotate source image 270 degrees clock-wise */
    NATIVE_WINDOW_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
};

// From Android native_handle.h.
typedef struct {
    int version;
    int num_fds;
    int num_ints;
} NativeHandle;
