/**
 * @file framebuffer.h
 * @brief Framebuffer wrapper object, providing support for software rendered graphics.
 * @author fincs
 * @copyright libnx Authors
 */
#pragma once
#include "../nvidia/map.h"
#include "native_window.h"

/// Framebuffer structure.
typedef struct Framebuffer {
    NWindow *win;
    NvMap map;
    void* buf;
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
 * @note Presently, only the following pixel formats are supported:
 *       \ref PIXEL_FORMAT_RGBA_8888
 *       \ref PIXEL_FORMAT_RGBX_8888
 *       \ref PIXEL_FORMAT_RGB_565
 *       \ref PIXEL_FORMAT_BGRA_8888
 *       \ref PIXEL_FORMAT_RGBA_4444
 */
Result framebufferCreate(Framebuffer* fb, NWindow *win, u32 width, u32 height, u32 format, u32 num_fbs);

/// Closes a \ref Framebuffer object, freeing all resources associated with it.
void framebufferClose(Framebuffer* fb);

/**
 * @brief Begins rendering a frame in a Framebuffer.
 * @param[in] fb Pointer to \ref Framebuffer structure.
 * @param[out] out_stride Output variable containing the distance in bytes between rows of pixels in memory.
 * @return Pointer to buffer to which new graphics data should be written to.
 * @note When this function is called, a buffer will be dequeued from the corresponding \ref NWindow.
 * @note This function will return pointers to different buffers, depending on the number of buffers it was initialized with.
 * @note Each call to \ref framebufferBegin must be paired with a \ref framebufferEnd call.
 */
void* framebufferBegin(Framebuffer* fb, u32* out_stride);

/**
 * @brief Finishes rendering a frame in a Framebuffer.
 * @param[in] fb Pointer to \ref Framebuffer structure.
 * @note When this function is called, the written image data will be flushed and queued (presented) in the corresponding \ref NWindow.
 * @note Each call to \ref framebufferBegin must be paired with a \ref framebufferEnd call.
 */
void framebufferEnd(Framebuffer* fb);
