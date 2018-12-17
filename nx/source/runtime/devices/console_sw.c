#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>
#include "result.h"
#include "runtime/devices/console.h"
#include "display/native_window.h"
#include "display/framebuffer.h"

//set up the palette for color printing
static const u32 colorTable[] = {
    RGBA8_MAXALPHA(  0,  0,  0),	// black
    RGBA8_MAXALPHA(128,  0,  0),	// red
    RGBA8_MAXALPHA(  0,128,  0),	// green
    RGBA8_MAXALPHA(128,128,  0),	// yellow
    RGBA8_MAXALPHA(  0,  0,128),	// blue
    RGBA8_MAXALPHA(128,  0,128),	// magenta
    RGBA8_MAXALPHA(  0,128,128),	// cyan
    RGBA8_MAXALPHA(192,192,192),	// white

    RGBA8_MAXALPHA(128,128,128),	// bright black
    RGBA8_MAXALPHA(255,  0,  0),	// bright red
    RGBA8_MAXALPHA(  0,255,  0),	// bright green
    RGBA8_MAXALPHA(255,255,  0),	// bright yellow
    RGBA8_MAXALPHA(  0,  0,255),	// bright blue
    RGBA8_MAXALPHA(255,  0,255),	// bright magenta
    RGBA8_MAXALPHA(  0,255,255),	// bright cyan
    RGBA8_MAXALPHA(255,255,255),	// bright white

    RGBA8_MAXALPHA(  0,  0,  0),	// faint black
    RGBA8_MAXALPHA( 64,  0,  0),	// faint red
    RGBA8_MAXALPHA(  0, 64,  0),	// faint green
    RGBA8_MAXALPHA( 64, 64,  0),	// faint yellow
    RGBA8_MAXALPHA(  0,  0, 64),	// faint blue
    RGBA8_MAXALPHA( 64,  0, 64),	// faint magenta
    RGBA8_MAXALPHA(  0, 64, 64),	// faint cyan
    RGBA8_MAXALPHA( 96, 96, 96),	// faint white
};

struct ConsoleSwRenderer
{
    ConsoleRenderer base;
    Framebuffer fb;          ///< Framebuffer object
    u32 *frameBuffer;        ///< Framebuffer address
    u32 frameBufferStride;   ///< Framebuffer stride (in pixels)
    bool initialized;
};

static struct ConsoleSwRenderer* ConsoleSwRenderer(PrintConsole* con)
{
    return (struct ConsoleSwRenderer*)con->renderer;
}

static bool ConsoleSwRenderer_init(PrintConsole* con)
{
    struct ConsoleSwRenderer* sw = ConsoleSwRenderer(con);

    if (con->font.tileWidth != 16 || con->font.tileHeight != 16) {
        // Only 16x16 tiles are supported for the font
        return false;
    }

    if (sw->initialized) {
        // We're already initialized
        return true;
    }

    NWindow* win = nwindowGetDefault();
    u32 width = con->font.tileWidth * con->consoleWidth;
    u32 height = con->font.tileHeight * con->consoleHeight;

    if (R_FAILED(nwindowSetDimensions(win, width, height))) {
        // Failed to set dimensions
        return false;
    }

    if (R_FAILED(framebufferCreate(&sw->fb, win, width, height, PIXEL_FORMAT_RGBA_8888, 2))) {
        // Failed to create framebuffer
        return false;
    }

    if (R_FAILED(framebufferMakeLinear(&sw->fb))) {
        // Failed to make framebuffer linear
        framebufferClose(&sw->fb);
        return false;
    }

    sw->frameBuffer = NULL;
    sw->frameBufferStride = 0;
    sw->initialized = true;

    return true;
}

static u32* _getFrameBuffer(struct ConsoleSwRenderer* sw, u32* out_stride)
{
    if (!sw->frameBuffer) {
        sw->frameBuffer = (u32*)framebufferBegin(&sw->fb, &sw->frameBufferStride);
        sw->frameBufferStride /= sizeof(u32);
    }
    if (out_stride)
        *out_stride = sw->frameBufferStride;
    return sw->frameBuffer;
}

static void ConsoleSwRenderer_drawChar(PrintConsole* con, int x, int y, int c)
{
    struct ConsoleSwRenderer* sw = ConsoleSwRenderer(con);
    u32 stride;
    u32* frameBuffer = _getFrameBuffer(sw, &stride);
    const u16 *fontdata = (const u16*)con->font.gfx + (16 * c);

    int writingColor = con->fg;
    int screenColor = con->bg;

    if (con->flags & CONSOLE_COLOR_BOLD) {
        writingColor += 8;
    } else if (con->flags & CONSOLE_COLOR_FAINT) {
        writingColor += 16;
    }

    if (con->flags & CONSOLE_COLOR_REVERSE) {
        int tmp = writingColor;
        writingColor = screenColor;
        screenColor = tmp;
    }

    u32 bg = colorTable[screenColor];
    u32 fg = colorTable[writingColor];

    u128 *tmp = (u128*)fontdata;

    u128 bvaltop = tmp[0];
    u128 bvalbtm = tmp[1];

    if (con->flags & CONSOLE_UNDERLINE)  bvalbtm |= (u128)0xffffULL << 7*16;

    if (con->flags & CONSOLE_CROSSED_OUT) bvaltop |= (u128)0xffffULL << 7*16;

    u16 mask = 0x8000;

    int i, j;

    x *= 16;
    y *= 16;

    u32 *screen;

    for (i=0;i<16;i++) {
        for (j=0;j<8;j++) {
            uint32_t screenOffset = (x + i) + stride*(y + j);
            screen = &frameBuffer[screenOffset];
            if (bvaltop >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }

            screenOffset = (x + i) + stride*(y + j + 8);
            screen = &frameBuffer[screenOffset];
            if (bvalbtm >> (16*j) & mask) { *screen = fg; }else{ *screen = bg; }
        }
        mask >>= 1;
    }
}

static void ConsoleSwRenderer_scrollWindow(PrintConsole* con)
{
    struct ConsoleSwRenderer* sw = ConsoleSwRenderer(con);
    u32 stride;
    u32* frameBuffer = _getFrameBuffer(sw, &stride);
    int i,j;
    u32 x, y;

    x = con->windowX * 16;
    y = con->windowY * 16;

    for (i=0; i<con->windowWidth*16; i++) {
        u32 *from;
        u32 *to;
        for (j=0;j<(con->windowHeight-1)*16;j++) {
            to = &frameBuffer[(x + i) + stride*(y + j)];
            from = &frameBuffer[(x + i) + stride*(y + 16 + j)];
            *to = *from;
        }
    }
}

static void ConsoleSwRenderer_flushAndSwap(PrintConsole* con)
{
    struct ConsoleSwRenderer* sw = ConsoleSwRenderer(con);
    _getFrameBuffer(sw, NULL); // Make sure we dequeued

    framebufferEnd(&sw->fb);
    sw->frameBuffer = NULL;
    sw->frameBufferStride = 0;
}


static void ConsoleSwRenderer_deinit(PrintConsole* con)
{
    struct ConsoleSwRenderer* sw = ConsoleSwRenderer(con);

    if (sw->initialized) {
        if (sw->frameBuffer)
            ConsoleSwRenderer_flushAndSwap(con);

        framebufferClose(&sw->fb);
        sw->initialized = false;
    }
}

static struct ConsoleSwRenderer s_consoleSwRenderer =
{
    {
        ConsoleSwRenderer_init,
        ConsoleSwRenderer_deinit,
        ConsoleSwRenderer_drawChar,
        ConsoleSwRenderer_scrollWindow,
        ConsoleSwRenderer_flushAndSwap,
    }
};

__attribute__((weak)) ConsoleRenderer* getDefaultConsoleRenderer(void)
{
    return &s_consoleSwRenderer.base;
}
