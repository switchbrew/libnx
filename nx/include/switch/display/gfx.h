/**
 * @file gfx.h
 * @brief High-level graphics API.
 * This API exposes a framebuffer (technically speaking, a windowbuffer) to be used for drawing graphics.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Converts red, green, blue, and alpha components to packed RGBA8.
#define RGBA8(r,g,b,a)  (((r)&0xff)|(((g)&0xff)<<8)|(((b)&0xff)<<16)|(((a)&0xff)<<24))

/// Same as \ref RGBA8 except with alpha=0xff.
#define RGBA8_MAXALPHA(r,g,b) RGBA8(r,g,b,0xff)

/// GfxMode set by \ref gfxSetMode. The default is GfxMode_LinearDouble. Note that the text-console (see console.h) sets this to GfxMode_TiledDouble.
typedef enum
{
    GfxMode_TiledSingle, ///< Single-buffering with raw tiled (block-linear) framebuffer.
    GfxMode_TiledDouble, ///< Double-buffering with raw tiled (block-linear) framebuffer.
    GfxMode_LinearDouble ///< Double-buffering with linear framebuffer, which is transferred to the actual framebuffer by \ref gfxFlushBuffers().
} GfxMode;

/// Framebuffer pixel-format is RGBA8888, there's no known way to change this.

/**
 * @brief Initializes the graphics subsystem.
 * @warning Do not use \ref viInitialize when using this function.
 */
Result gfxInitDefault(void);

/**
 * @brief Uninitializes the graphics subsystem.
 * @warning Do not use \ref viExit when using this function.
 */
void gfxExit(void);

/**
 * @brief Sets the resolution to be used when initializing the graphics subsystem.
 * @param[in] width Horizontal resolution, in pixels.
 * @param[in] height Vertical resolution, in pixels.
 * @note The default resolution is 720p.
 * @note This can only be used before calling \ref gfxInitDefault, this will use \ref fatalSimple otherwise. If the input is 0, the default resolution will be used during \ref gfxInitDefault. This sets the maximum resolution for the framebuffer, used during \ref gfxInitDefault. This is also used as the current resolution when crop isn't set. The width/height are reset to the default when \ref gfxExit is used.
 * @note Normally you should only use this when you need a maximum resolution larger than the default, see above.
 * @note The width and height are aligned to 4.
 */
void gfxInitResolution(u32 width, u32 height);

/// Wrapper for \ref gfxInitResolution with resolution=1080p. Use this if you want to support 1080p or >720p in docked-mode.
void gfxInitResolutionDefault(void);

/// Configure framebuffer crop, by default crop is all-zero. Use all-zero input to reset to default. \ref gfxExit resets this to the default.
/// When the input is invalid this returns without changing the crop data, this includes the input values being larger than the framebuf width/height.
/// This will update the display width/height returned by \ref gfxGetFramebuffer, with that width/height being reset to the default when required.
/// \ref gfxGetFramebufferDisplayOffset uses absolute x/y, it will not adjust for non-zero crop left/top.
/// The new crop config will not take affect with double-buffering disabled. When used during frame-drawing, this should be called before \ref gfxGetFramebuffer.
/// The right and bottom params are aligned to 4.
void gfxConfigureCrop(s32 left, s32 top, s32 right, s32 bottom);

/// Wrapper for \ref gfxConfigureCrop. Use this to set the resolution, within the bounds of the maximum resolution. Use all-zero input to reset to default.
void gfxConfigureResolution(s32 width, s32 height);

/// If enabled, \ref gfxConfigureResolution will be used with the input resolution for the current OperationMode. Then \ref gfxConfigureResolution will automatically be used with the specified resolution each time OperationMode changes.
void gfxConfigureAutoResolution(bool enable, s32 handheld_width, s32 handheld_height, s32 docked_width, s32 docked_height);

/// Wrapper for \ref gfxConfigureAutoResolution. handheld_resolution=720p, docked_resolution={all-zero for using current maximum resolution}.
void gfxConfigureAutoResolutionDefault(bool enable);

/// Waits for vertical sync.
void gfxWaitForVsync(void);

/// Swaps the framebuffers (for double-buffering).
void gfxSwapBuffers(void);

/// Get the current framebuffer address, with optional output ptrs for the display framebuffer width/height. The display width/height is adjusted by \ref gfxConfigureCrop and \ref gfxConfigureResolution.
u8* gfxGetFramebuffer(u32* width, u32* height);

/// Get the framebuffer width/height without crop.
void gfxGetFramebufferResolution(u32* width, u32* height);

/// Use this to get the actual byte-size of the framebuffer for use with memset/etc.
size_t gfxGetFramebufferSize(void);

/// Sets the \ref GfxMode.
void gfxSetMode(GfxMode mode);

/// Controls whether a vertical-flip is done when determining the pixel-offset within the actual framebuffer. By default this is enabled.
void gfxSetDrawFlip(bool flip);

/// Configures transform. See the NATIVE_WINDOW_TRANSFORM_* enums in buffer_producer.h. The default is NATIVE_WINDOW_TRANSFORM_FLIP_V.
void gfxConfigureTransform(u32 transform);

/// Flushes the framebuffer in the data cache. When \ref GfxMode is GfxMode_LinearDouble, this also transfers the linear-framebuffer to the actual framebuffer.
void gfxFlushBuffers(void);

/// Use this to get the pixel-offset in the framebuffer. Returned value is in pixels, not bytes.
/// This implements tegra blocklinear, with hard-coded constants etc.
/// Do not use this when \ref GfxMode is GfxMode_LinearDouble.
static inline u32 gfxGetFramebufferDisplayOffset(u32 x, u32 y) {
    u32 tmp_pos;

    extern size_t g_gfx_framebuf_aligned_width;
    extern size_t g_gfx_framebuf_display_height;
    extern bool g_gfx_drawflip;

    //if (x >= g_gfx_framebuf_width || y >= g_gfx_framebuf_display_height) return (gfxGetFramebufferSize()-4)/4;//Return the last pixel-offset in the buffer, the data located here is not displayed due to alignment. (Disabled for perf)

    if (g_gfx_drawflip) y = g_gfx_framebuf_display_height-1-y;

    tmp_pos = ((y & 127) / 16) + (x/16*8) + ((y/16/8)*(g_gfx_framebuf_aligned_width/16*8));
    tmp_pos *= 16*16 * 4;

    tmp_pos += ((y%16)/8)*512 + ((x%16)/8)*256 + ((y%8)/2)*64 + ((x%8)/4)*32 + (y%2)*16 + (x%4)*4;//This line is a modified version of code from the Tegra X1 datasheet.

    return tmp_pos / 4;
}
