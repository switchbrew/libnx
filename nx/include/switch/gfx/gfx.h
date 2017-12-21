/// Converts red, green, blue, and alpha components to packed RGBA8.
#define RGBA8(r,g,b,a)  (((r)&0xff)|(((g)&0xff)<<8)|(((b)&0xff)<<16)|(((a)&0xff)<<24))

/// Same as RGBA8 except with alpha=0xff.
#define RGBA8_MAXALPHA(r,g,b) RGBA8(r,g,b,0xff)

/// Do not use viInitialize/viExit when using these.
void gfxInitDefault(void);
void gfxExit(void);

/// Note that "framebuffer" here is technically windowbuffer.

/// The default resolution is 720p, however you should use gfxGetFramebuffer() to get the current width/height.

/// This can only be used before calling gfxInitDefault(), this will use fatalSimple() otherwise. If the input is 0, the default resolution will be used during gfxInitDefault(). This sets the maximum resolution for the framebuffer, used during gfxInitDefault(). This is also used as the current resolution. The width/height are reset to the default when gfxExit() is used.
void gfxInitResolution(u32 width, u32 height);

void gfxWaitForVsync();
void gfxSwapBuffers();
u8* gfxGetFramebuffer(u32* width, u32* height);
size_t gfxGetFramebufferSize(void); /// Use this to get the actual byte-size of the buffer for use with memset/etc, do not calculate the byte-size manually with the width and height from gfxGetFramebuffer. The height returned by gfxGetFramebuffer is the display height not the aligned height.
void gfxSetDoubleBuffering(bool doubleBuffering);
void gfxFlushBuffers(void);

/// Use this to get the pixel-offset in the framebuffer. Returned value is in pixels, not bytes.
/// This implements tegra blocklinear, with hard-coded constants etc.
static inline u32 gfxGetFramebufferDisplayOffset(u32 x, u32 y) {
    u32 width=0, height=0;
    u32 tilepos, tmp_pos;

    gfxGetFramebuffer(&width, &height);

    if (x >= width || y >= height) return (gfxGetFramebufferSize()-4)/4;//Return the last pixel-offset in the buffer, the data located here is not displayed due to alignment.

    y = height-1-y;

    tilepos = ((y & 127) / 16) + (x/16*8) + ((y/16/8)*(width/16*8));
    tilepos = tilepos*16*16 * 4;

    tmp_pos = ((y%16)/8)*512 + ((x%16)/8)*256 + ((y%8)/2)*64 + ((x%8)/4)*32 + (y%2)*16 + (x%4)*4;//This line is a modified version of code from the Tegra X1 datasheet.

    return (tilepos + tmp_pos) / 4;
}

