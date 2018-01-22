#pragma once
#include "types.h"

/// Converts red, green, blue, and alpha components to packed RGBA8.
#define RGBA8(r,g,b,a)  (((r)&0xff)|(((g)&0xff)<<8)|(((b)&0xff)<<16)|(((a)&0xff)<<24))

/// Same as RGBA8 except with alpha=0xff.
#define RGBA8_MAXALPHA(r,g,b) RGBA8(r,g,b,0xff)

/// Do not use viInitialize/viExit when using these.
void gfxInitDefault(void);
void gfxExit(void);

/// Note that "framebuffer" here is technically windowbuffer.

/// The default resolution is 720p, however you should use gfxGetFramebuffer() to get the current width/height.

/// This can only be used before calling gfxInitDefault(), this will use fatalSimple() otherwise. If the input is 0, the default resolution will be used during gfxInitDefault(). This sets the maximum resolution for the framebuffer, used during gfxInitDefault(). This is also used as the current resolution when crop isn't set. The width/height are reset to the default when gfxExit() is used.
/// Normally you should only use this when you need a maximum resolution larger than the default, see above.
void gfxInitResolution(u32 width, u32 height);

/// Wrapper for gfxInitResolution() with resolution=1080p. Use this if you want to support 1080p or >720p in docked-mode.
void gfxInitResolutionDefault(void);

/// Configure framebuffer crop, by default crop is all-zero. Use all-zero input to reset to default. gfxExit() resets this to the default.
/// When the input is invalid this returns without changing the crop data, this includes the input values being larger than the framebuf width/height.
/// This will update the display width/height returned by gfxGetFramebuffer(), with that width/height being reset to the default when required.
/// gfxGetFramebufferDisplayOffset() uses absolute x/y, it will not adjust for non-zero crop left/top.
/// The new crop config will not take affect with double-buffering disabled. When used during frame-drawing, this should be called before gfxGetFramebuffer().
void gfxConfigureCrop(s32 left, s32 top, s32 right, s32 bottom);

/// Wrapper for gfxConfigureCrop(). Use this to set the resolution, within the bounds of the maximum resolution. Use all-zero input to reset to default.
void gfxConfigureResolution(s32 width, s32 height);

/// If enabled, gfxConfigureResolution() will be used with the input resolution for the current OperationMode. Then gfxConfigureResolution() will automatically be used with the specified resolution each time OperationMode changes.
void gfxConfigureAutoResolution(bool enable, s32 handheld_width, s32 handheld_height, s32 docked_width, s32 docked_height);

/// Wrapper for gfxConfigureAutoResolution(). handheld_resolution=720p, docked_resolution={all-zero for using current maximum resolution}.
void gfxConfigureAutoResolutionDefault(bool enable);

void gfxWaitForVsync(void);
void gfxSwapBuffers(void);

/// Get the current framebuffer address, with optional output ptrs for the display width/height. The display width/height is adjusted by gfxConfigureCrop()/gfxConfigureResolution().
u8* gfxGetFramebuffer(u32* width, u32* height);

/// Get the original framebuffer width/height without crop.
void gfxGetFramebufferResolution(u32* width, u32* height);

/// Use this to get the actual byte-size of the buffer for use with memset/etc, do not calculate the byte-size manually with the width and height from gfxGetFramebuffer/gfxGetFramebufferResolution.
size_t gfxGetFramebufferSize(void);

void gfxSetDoubleBuffering(bool doubleBuffering);
void gfxFlushBuffers(void);

/// Use this to get the pixel-offset in the framebuffer. Returned value is in pixels, not bytes.
/// This implements tegra blocklinear, with hard-coded constants etc.
static inline u32 gfxGetFramebufferDisplayOffset(u32 x, u32 y) {
    u32 tmp_pos;

    extern size_t g_gfx_framebuf_width;
    extern size_t g_gfx_framebuf_display_height;

    //if (x >= g_gfx_framebuf_width || y >= g_gfx_framebuf_display_height) return (gfxGetFramebufferSize()-4)/4;//Return the last pixel-offset in the buffer, the data located here is not displayed due to alignment. (Disabled for perf)

    y = g_gfx_framebuf_display_height-1-y;

    tmp_pos = ((y & 127) / 16) + (x/16*8) + ((y/16/8)*(g_gfx_framebuf_width/16*8));
    tmp_pos *= 16*16 * 4;

    tmp_pos += ((y%16)/8)*512 + ((x%16)/8)*256 + ((y%8)/2)*64 + ((x%8)/4)*32 + (y%2)*16 + (x%4)*4;//This line is a modified version of code from the Tegra X1 datasheet.

    return tmp_pos / 4;
}

