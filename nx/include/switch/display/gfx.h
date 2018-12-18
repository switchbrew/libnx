/**
 * @file gfx.h
 * @brief Deprecated graphics API, use \ref NWindow and \ref Framebuffer instead.
 * @deprecated Use \ref NWindow and \ref Framebuffer instead.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "../nvidia/fence.h"

/// GfxMode set by \ref gfxSetMode. The default is GfxMode_LinearDouble. Note that the text-console (see console.h) sets this to GfxMode_TiledDouble.
/// \deprecated Use \ref framebufferMakeLinear instead.
typedef enum
{
    GfxMode_TiledDouble, ///< Double-buffering with raw tiled (block-linear) framebuffer.
    GfxMode_LinearDouble ///< Double-buffering with linear framebuffer, which is transferred to the actual framebuffer by \ref gfxFlushBuffers().
} GfxMode;

/**
 * @brief Initializes the deprecated graphics subsystem.
 * @warning Do not use \ref viInitialize when using this function.
 * @deprecated Use \ref nwindowGetDefault and \ref framebufferCreate instead.
 */
__attribute__((deprecated))
Result gfxInitDefault(void);

/**
 * @brief Uninitializes the deprecated graphics subsystem.
 * @warning Do not use \ref viExit when using this function.
 * @deprecated Use \ref framebufferClose instead.
 */
__attribute__((deprecated))
void gfxExit(void);

/**
 * @brief Sets the resolution to be used when initializing the deprecated graphics subsystem.
 * @param[in] width Horizontal resolution, in pixels.
 * @param[in] height Vertical resolution, in pixels.
 * @note The default resolution is 720p.
 * @note This can only be used before calling \ref gfxInitDefault, this will use \ref fatalSimple otherwise. If the input is 0, the default resolution will be used during \ref gfxInitDefault. This sets the maximum resolution for the framebuffer, used during \ref gfxInitDefault. This is also used as the current resolution when crop isn't set. The width/height are reset to the default when \ref gfxExit is used.
 * @note Normally you should only use this when you need a maximum resolution larger than the default, see above.
 * @note The width and height are aligned to 4.
 * @deprecated Use \ref nwindowSetDimensions instead.
 */
__attribute__((deprecated))
void gfxInitResolution(u32 width, u32 height);

/// Wrapper for \ref gfxInitResolution with resolution=1080p. Use this if you want to support 1080p or >720p in docked-mode.
/// \deprecated Use \ref nwindowSetDimensions instead.
__attribute__((deprecated))
void gfxInitResolutionDefault(void);

/// Configure framebuffer crop, by default crop is all-zero. Use all-zero input to reset to default. \ref gfxExit resets this to the default.
/// When the input is invalid this returns without changing the crop data, this includes the input values being larger than the framebuf width/height.
/// This will update the display width/height returned by \ref gfxGetFramebuffer, with that width/height being reset to the default when required.
/// \ref gfxGetFramebufferDisplayOffset uses absolute x/y, it will not adjust for non-zero crop left/top.
/// When used during frame-drawing, this should be called before \ref gfxGetFramebuffer.
/// The right and bottom params are aligned to 4.
/// \deprecated Use \ref nwindowSetCrop instead.
__attribute__((deprecated))
void gfxConfigureCrop(s32 left, s32 top, s32 right, s32 bottom);

/// Wrapper for \ref gfxConfigureCrop. Use this to set the resolution, within the bounds of the maximum resolution. Use all-zero input to reset to default.
/// \deprecated Use \ref nwindowSetCrop instead.
__attribute__((deprecated))
void gfxConfigureResolution(s32 width, s32 height);

/// If enabled, \ref gfxConfigureResolution will be used with the input resolution for the current OperationMode. Then \ref gfxConfigureResolution will automatically be used with the specified resolution each time OperationMode changes.
/// \deprecated No replacement. Manually query the current OperationMode and use \ref nwindowSetCrop instead.
__attribute__((deprecated))
void gfxConfigureAutoResolution(bool enable, s32 handheld_width, s32 handheld_height, s32 docked_width, s32 docked_height);

/// Wrapper for \ref gfxConfigureAutoResolution. handheld_resolution=720p, docked_resolution={all-zero for using current maximum resolution}.
/// \deprecated No replacement. Manually query the current OperationMode and use \ref nwindowSetCrop instead.
__attribute__((deprecated))
void gfxConfigureAutoResolutionDefault(bool enable);

/// Waits for vertical sync.
/// \deprecated No replacement. Waiting for vertical sync is neither necessary nor desirable with the system's compositor.
__attribute__((deprecated))
void gfxWaitForVsync(void);

/// Appends one or more fences that the display service will wait on before rendering the current framebuffer. Note that only up to 4 fences can be submitted.
/// \deprecated Use \ref nwindowQueueBuffer parameter \p fence instead.
__attribute__((deprecated))
void gfxAppendFence(NvMultiFence* mf);

/// Swaps the framebuffers.
/// \deprecated Use \ref framebufferBegin and \ref framebufferEnd, or \ref nwindowDequeueBuffer and \ref nwindowQueueBuffer instead.
__attribute__((deprecated))
void gfxSwapBuffers(void);

/// Get the specified framebuffer nvmap handle where index specifies the buffer number beginning with the back buffer, with optional output ptr for the offset in the buffer.
/// \deprecated No replacement.
__attribute__((deprecated))
u32 gfxGetFramebufferHandle(u32 index, u32* offset);

/// Get the current framebuffer address, with optional output ptrs for the display framebuffer width/height. The display width/height is adjusted by \ref gfxConfigureCrop and \ref gfxConfigureResolution.
/// \deprecated Check return value of \ref framebufferBegin instead.
__attribute__((deprecated))
u8* gfxGetFramebuffer(u32* width, u32* height);

/// Get the framebuffer width/height without crop.
/// \deprecated Use \ref nwindowGetDimensions instead.
__attribute__((deprecated))
void gfxGetFramebufferResolution(u32* width, u32* height);

/// Use this to get the actual byte-size of the framebuffer for use with memset/etc.
/// \deprecated No replacement.
__attribute__((deprecated))
size_t gfxGetFramebufferSize(void);

/// Use this to get the actual byte-pitch of the framebuffer for use with memset/etc.
/// \deprecated Use \ref framebufferBegin parameter \p out_stride instead.
__attribute__((deprecated))
u32 gfxGetFramebufferPitch(void);

/// Sets the \ref GfxMode.
/// \deprecated Use \ref framebufferMakeLinear instead.
__attribute__((deprecated))
void gfxSetMode(GfxMode mode);

/// Configures transform. See the NATIVE_WINDOW_TRANSFORM_* enums in buffer_producer.h. The default is 0.
/// \deprecated Use \ref nwindowSetTransform instead.
__attribute__((deprecated))
void gfxConfigureTransform(u32 transform);

/// Flushes the framebuffer in the data cache. When \ref GfxMode is GfxMode_LinearDouble, this also transfers the linear-framebuffer to the actual framebuffer.
/// \deprecated No replacement and no need to flush data cache manually when using \ref Framebuffer.
__attribute__((deprecated))
void gfxFlushBuffers(void);

/// Use this to get the pixel-offset in the framebuffer. Returned value is in pixels, not bytes.
/// This implements tegra blocklinear, with hard-coded constants etc.
/// Do not use this when \ref GfxMode is GfxMode_LinearDouble.
/// \deprecated No replacement. Use linear mode (\ref framebufferMakeLinear), or manually implement Tegra block linear layout with 16Bx2 sector ordering.
__attribute__((deprecated))
static inline u32 gfxGetFramebufferDisplayOffset(u32 x, u32 y) {
    u32 tmp_pos;

    extern size_t g_gfx_framebuf_aligned_width;

    //if (x >= g_gfx_framebuf_width || y >= g_gfx_framebuf_display_height) return (gfxGetFramebufferSize()-4)/4;//Return the last pixel-offset in the buffer, the data located here is not displayed due to alignment. (Disabled for perf)

    tmp_pos = ((y & 127) / 16) + (x/16*8) + ((y/16/8)*(g_gfx_framebuf_aligned_width/16*8));
    tmp_pos *= 16*16 * 4;

    tmp_pos += ((y%16)/8)*512 + ((x%16)/8)*256 + ((y%8)/2)*64 + ((x%8)/4)*32 + (y%2)*16 + (x%4)*4;//This line is a modified version of code from the Tegra X1 datasheet.

    return tmp_pos / 4;
}
