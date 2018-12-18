/**
 * @file framebuffer.h
 * @brief Framebuffer wrapper object, providing support for software rendered graphics.
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../nvidia/map.h"
#include "native_window.h"

/// Converts red/green/blue/alpha components to packed RGBA8 (i.e. \ref PIXEL_FORMAT_RGBA_8888).
#define RGBA8(r,g,b,a) (((r)&0xff)|(((g)&0xff)<<8)|(((b)&0xff)<<16)|(((a)&0xff)<<24))

/// Same as \ref RGBA8 except with alpha=0xff.
#define RGBA8_MAXALPHA(r,g,b) RGBA8((r),(g),(b),0xff)

/// Converts red/green/blue to packed RGBX8 (i.e. \ref PIXEL_FORMAT_RGBX_8888).
#define RGBX8(r,g,b) RGBA8((r),(g),(b),0)

/// Converts red/green/blue components to packed RGB565 (i.e. \ref PIXEL_FORMAT_RGB_565)
#define RGB565(r,g,b) (((b)&0x1f)|(((g)&0x3f)<<5)|(((r)&0x1f)<<11))

/// Same as \ref RGB565 but accepting 8-bit components as input instead.
#define RGB565_FROM_RGB8(r,g,b) RGB565((r)>>3,(g)>>2,(b)>>3)

/// Converts red/green/blue/alpha components to packed BGR8 (i.e. \ref PIXEL_FORMAT_BGRA_8888).
#define BGRA8(r,g,b,a) RGBA8((b),(g),(r),(a))

/// Same as \ref BGRA8 except with alpha=0xff.
#define BGRA8_MAXALPHA(r,g,b) RGBA8((b),(g),(r),0xff)

/// Converts red/green/blue/alpha components to packed RGBA4 (i.e. \ref PIXEL_FORMAT_RGBA_4444).
#define RGBA4(r,g,b,a) (((r)&0xf)|(((g)&0xf)<<4)|(((b)&0xf)<<8)|(((a)&0xf)<<12))

/// Same as \ref RGBA4 except with alpha=0xf.
#define RGBA4_MAXALPHA(r,g,b) RGBA4((r),(g),(b),0xf)

/// Same as \ref RGBA4 but accepting 8-bit components as input instead.
#define RGBA4_FROM_RGBA8(r,g,b,a) RGBA4((r)>>4,(g)>>4,(b)>>4,(a)>>4)

/// Same as \ref RGBA4_MAXALPHA except with alpha=0xff.
#define RGBA4_FROM_RGBA8_MAXALPHA(r,g,b) RGBA4_MAXALPHA((r)>>4,(g)>>4,(b)>>4)

/// Framebuffer structure.
typedef struct Framebuffer {
    NWindow *win;
    NvMap map;
    void* buf;
    void* buf_linear;
    u32 stride;
    u32 width_aligned;
    u32 height_aligned;
    u32 num_fbs;
    u32 fb_size;
    bool has_init;
} Framebuffer;

/**
 * @brief Creates a \ref Framebuffer object.
 * @param[out] fb Output \ref Framebuffer structure.
 * @param[in] win Pointer to the \ref NWindow to which the \ref Framebuffer will be registered.
 * @param[in] width Desired width of the framebuffer (usually 1280).
 * @param[in] height Desired height of the framebuffer (usually 720).
 * @param[in] format Desired pixel format (see PIXEL_FORMAT_* enum).
 * @param[in] num_fbs Number of buffers to create. Pass 1 for single-buffering, 2 for double-buffering or 3 for triple-buffering.
 * @note Framebuffer images are stored in Tegra block linear format with 16Bx2 sector ordering, read TRM chapter 20.1 for more details.
 *       In order to use regular linear layout, consider calling \ref framebufferMakeLinear after the \ref Framebuffer object is created.
 * @note Presently, only the following pixel formats are supported:
 *       \ref PIXEL_FORMAT_RGBA_8888
 *       \ref PIXEL_FORMAT_RGBX_8888
 *       \ref PIXEL_FORMAT_RGB_565
 *       \ref PIXEL_FORMAT_BGRA_8888
 *       \ref PIXEL_FORMAT_RGBA_4444
 */
Result framebufferCreate(Framebuffer* fb, NWindow *win, u32 width, u32 height, u32 format, u32 num_fbs);

/// Enables linear framebuffer mode in a \ref Framebuffer, allocating a shadow buffer in the process.
Result framebufferMakeLinear(Framebuffer* fb);

/// Closes a \ref Framebuffer object, freeing all resources associated with it.
void framebufferClose(Framebuffer* fb);

/**
 * @brief Begins rendering a frame in a \ref Framebuffer.
 * @param[in] fb Pointer to \ref Framebuffer structure.
 * @param[out] out_stride Output variable containing the distance in bytes between rows of pixels in memory.
 * @return Pointer to buffer to which new graphics data should be written to.
 * @note When this function is called, a buffer will be dequeued from the corresponding \ref NWindow.
 * @note This function will return pointers to different buffers, depending on the number of buffers it was initialized with.
 * @note If \ref framebufferMakeLinear was used, this function will instead return a pointer to the shadow linear buffer.
 *       In this case, the offset of a pixel is \p y * \p out_stride + \p x * \p bytes_per_pixel.
 * @note Each call to \ref framebufferBegin must be paired with a \ref framebufferEnd call.
 */
void* framebufferBegin(Framebuffer* fb, u32* out_stride);

/**
 * @brief Finishes rendering a frame in a \ref Framebuffer.
 * @param[in] fb Pointer to \ref Framebuffer structure.
 * @note When this function is called, the written image data will be flushed and queued (presented) in the corresponding \ref NWindow.
 * @note If \ref framebufferMakeLinear was used, this function will copy the image from the shadow linear buffer to the actual framebuffer,
 *       converting it in the process to the layout expected by the compositor.
 * @note Each call to \ref framebufferBegin must be paired with a \ref framebufferEnd call.
 */
void framebufferEnd(Framebuffer* fb);
