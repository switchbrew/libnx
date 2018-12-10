#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "services/fatal.h"
#include "services/nv.h"
#include "services/vi.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/native_window.h"
#include "display/framebuffer.h"
#include "nvidia/graphic_buffer.h"

static const NvColorFormat g_nvColorFmtTable[] = {
    NvColorFormat_A8B8G8R8, // PIXEL_FORMAT_RGBA_8888
    NvColorFormat_X8B8G8R8, // PIXEL_FORMAT_RGBX_8888
    NvColorFormat_R8_G8_B8, // PIXEL_FORMAT_RGB_888   <-- doesn't work
    NvColorFormat_R5G6B5,   // PIXEL_FORMAT_RGB_565
    NvColorFormat_A8R8G8B8, // PIXEL_FORMAT_BGRA_8888
    NvColorFormat_R5G5B5A1, // PIXEL_FORMAT_RGBA_5551 <-- doesn't work
    NvColorFormat_A4B4G4R4, // PIXEL_FORMAT_RGBA_4444
};

Result framebufferCreate(Framebuffer* fb, NWindow *win, u32 width, u32 height, u32 format, u32 num_fbs)
{
    Result rc = 0;
    if (!fb || !nwindowIsValid(win) || !width || !height || format < PIXEL_FORMAT_RGBA_8888 || format > PIXEL_FORMAT_RGBA_4444 || num_fbs < 1 || num_fbs > 3)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    rc = nvInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = nvMapInit();
        if (R_SUCCEEDED(rc)) {
            rc = nvFenceInit();
            if (R_FAILED(rc))
                nvMapExit();
        }
        if (R_FAILED(rc))
            nvExit();
    }

    if (R_FAILED(rc))
        return rc;

    memset(fb, 0, sizeof(*fb));
    fb->has_init = true;
    fb->win = win;
    fb->num_fbs = num_fbs;

    const NvColorFormat colorfmt = g_nvColorFmtTable[format-PIXEL_FORMAT_RGBA_8888];
    const u32 bytes_per_pixel = ((u64)colorfmt >> 3) & 0x1F;
    const u32 block_height_log2 = 4; // According to TRM this is the optimal value (SIXTEEN_GOBS)
    const u32 block_height = 8 * (1U << block_height_log2);

    NvGraphicBuffer grbuf = {0};
    grbuf.header.num_ints = (sizeof(NvGraphicBuffer) - sizeof(NativeHandle)) / 4;
    grbuf.unk0 = -1;
    grbuf.magic = 0xDAFFCAFF;
    grbuf.pid = 42;
    grbuf.usage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;
    grbuf.format = format;
    grbuf.ext_format = format;
    grbuf.num_planes = 1;
    grbuf.planes[0].width = width;
    grbuf.planes[0].height = height;
    grbuf.planes[0].color_format = colorfmt;
    grbuf.planes[0].layout = NvLayout_BlockLinear;
    grbuf.planes[0].kind = NvKind_Generic_16BX2;
    grbuf.planes[0].block_height_log2 = block_height_log2;

    // Calculate buffer dimensions and sizes
    const u32 width_aligned_bytes = (width*bytes_per_pixel + 63) &~ 63; // GOBs are 64 bytes wide
    const u32 width_aligned = width_aligned_bytes / bytes_per_pixel;
    const u32 height_aligned = (height + block_height - 1) &~ (block_height - 1);
    const u32 fb_size = width_aligned_bytes*height_aligned;
    const u32 buf_size = (num_fbs*fb_size + 0xFFF) &~ 0xFFF; // needs to be page aligned

    fb->buf = aligned_alloc(0x1000, buf_size);
    if (!fb->buf)
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    if (R_SUCCEEDED(rc))
        rc = nvMapCreate(&fb->map, fb->buf, buf_size, 0x20000, NvKind_Pitch, true);

    if (R_SUCCEEDED(rc)) {
        grbuf.nvmap_id = nvMapGetId(&fb->map);
        grbuf.stride = width_aligned;
        grbuf.total_size = fb_size;
        grbuf.planes[0].pitch = width_aligned_bytes;
        grbuf.planes[0].size = fb_size;

        for (u32 i = 0; i < num_fbs; i ++) {
            grbuf.planes[0].offset = i*fb_size;
            rc = nwindowConfigureBuffer(win, i, &grbuf);
            if (R_FAILED(rc))
                break;
        }
    }

    if (R_SUCCEEDED(rc)) {
        fb->stride = width_aligned_bytes;
        fb->width_aligned = width_aligned;
        fb->height_aligned = height_aligned;
        fb->fb_size = fb_size;
    }

    if (R_FAILED(rc))
        framebufferClose(fb);

    return rc;
}

void framebufferClose(Framebuffer* fb)
{
    if (!fb || !fb->has_init)
        return;

    if (fb->buf) {
        nwindowReleaseBuffers(fb->win);
        nvMapClose(&fb->map);
        free(fb->buf);
    }

    memset(fb, 0, sizeof(*fb));
    nvFenceExit();
    nvMapExit();
    nvExit();
}

void* framebufferBegin(Framebuffer* fb, u32* out_stride)
{
    if (!fb->has_init)
        return NULL;

    s32 slot;
    Result rc = nwindowDequeueBuffer(fb->win, &slot, NULL);
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxDequeueBuffer));

    if (out_stride)
        *out_stride = fb->stride;

    return (u8*)fb->buf + slot*fb->fb_size;
}

void framebufferEnd(Framebuffer* fb)
{
    if (!fb->has_init)
        return;

    void* buf = (u8*)fb->buf + fb->win->cur_slot*fb->fb_size;
    armDCacheFlush(buf, fb->fb_size);

    Result rc = nwindowQueueBuffer(fb->win, fb->win->cur_slot, NULL);
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxQueueBuffer));
}
