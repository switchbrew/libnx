#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "services/fatal.h"
#include "services/vi.h"
#include "services/applet.h"
#include "services/nv.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/gfx.h"
#include "nvidia/map.h"
#include "nvidia/graphic_buffer.h"

__attribute__((weak)) ViServiceType __nx_gfx_vi_service_type = ViServiceType_Default;

static bool g_gfxInitialized = 0;
static ViDisplay g_gfxDisplay;
static Event g_gfxDisplayVsyncEvent;
static ViLayer g_gfxLayer;
static Binder g_gfxBinderSession;
static Event g_gfxBinderEvent;
static s32 g_gfxCurrentBuffer = 0;
static s32 g_gfxCurrentProducerBuffer = 0;
static bool g_gfx_ProducerConnected = 0;
static u32 g_gfx_ProducerSlotsRequested = 0;
static u8 *g_gfxFramebuf;
static size_t g_gfxFramebufSize;
static BqBufferOutput g_gfx_Connect_QueueBufferOutput;
static BqBufferOutput g_gfx_QueueBuffer_QueueBufferOutput;

static GfxMode g_gfxMode = GfxMode_LinearDouble;

static u8 *g_gfxFramebufLinear;

size_t g_gfx_framebuf_width=0, g_gfx_framebuf_aligned_width=0;
size_t g_gfx_framebuf_height=0, g_gfx_framebuf_aligned_height=0;
size_t g_gfx_framebuf_display_width=0, g_gfx_framebuf_display_height=0;
size_t g_gfx_singleframebuf_size=0;
size_t g_gfx_singleframebuf_linear_size=0;

static AppletHookCookie g_gfx_autoresolution_applethookcookie;
static bool g_gfx_autoresolution_enabled;

static s32 g_gfx_autoresolution_handheld_width;
static s32 g_gfx_autoresolution_handheld_height;
static s32 g_gfx_autoresolution_docked_width;
static s32 g_gfx_autoresolution_docked_height;

extern u32 __nx_applet_type;

static const u32 g_nvgfx_totalframebufs = 2;
static NvMap g_nvmap_obj;

//static Result _gfxGetDisplayResolution(u64 *width, u64 *height);

// TODO: Let the user configure some of this?
static BqBufferInput g_gfxQueueBufferData = {
    .timestamp = 0x0,
    .isAutoTimestamp = 0x1,
    .crop = {0x0, 0x0, 0x0, 0x0}, //Official apps which use multiple resolutions configure this for the currently used resolution, depending on the current appletOperationMode.
    .scalingMode = 0x0,
    .transform = 0,
    .stickyTransform = 0x0,
    .unk = 0x0,
    .swapInterval = 0x1,
    .fence = {0},
};

static NvGraphicBuffer g_gfx_GraphicBuffer = {
    .header = {
        .num_ints = (sizeof(NvGraphicBuffer) - sizeof(NativeHandle)) / 4,
    },
    .unk0 = -1,
    .magic = 0xDAFFCAFF,
    .pid = 42,
    .usage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
    .format = PIXEL_FORMAT_RGBA_8888,
    .ext_format = PIXEL_FORMAT_RGBA_8888,
    .num_planes = 1,
    .planes = {
        {
            .color_format = 0x100532120UL, // this is 'A8B8G8R8' according to symbols in official sw
            .layout = NvLayout_BlockLinear,
            .kind = NvKind_Generic_16BX2,
            .block_height_log2 = 4, // i.e. block height is 16 which is the preferred value according to TRM
        }
    }
};

static BqGraphicBuffer g_gfx_BufferInitData = {
    .format = PIXEL_FORMAT_RGBA_8888,
    .usage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
    .native_handle = &g_gfx_GraphicBuffer.header,
};

static Result _gfxDequeueBuffer(void) {
    if (g_gfxCurrentProducerBuffer >= 0)
        return 0;

    NvMultiFence fence;
    s32 slot;
    Result rc;

    if (g_gfxBinderEvent.revent != INVALID_HANDLE) {
        do {
            eventWait(&g_gfxBinderEvent, U64_MAX);
            rc = bqDequeueBuffer(&g_gfxBinderSession, true, g_gfx_framebuf_width, g_gfx_framebuf_height, 0, 0x300, &slot, &fence);
        } while (rc == MAKERESULT(Module_LibnxBinder, LibnxBinderError_WouldBlock));
    }
    else
        rc = bqDequeueBuffer(&g_gfxBinderSession, false, g_gfx_framebuf_width, g_gfx_framebuf_height, 0, 0x300, &slot, &fence);

    if (R_FAILED(rc))
        return rc;

    if (!(g_gfx_ProducerSlotsRequested & BIT(slot))) {
        rc = bqRequestBuffer(&g_gfxBinderSession, slot, NULL);
        if (R_FAILED(rc)) {
            bqCancelBuffer(&g_gfxBinderSession, slot, &fence);
            return rc;
        }
        g_gfx_ProducerSlotsRequested |= BIT(slot);
    }

    rc = nvMultiFenceWait(&fence, -1);
    if (R_FAILED(rc))
        return rc;

    if (R_SUCCEEDED(rc)) {
        g_gfxCurrentProducerBuffer = slot;
        g_gfxCurrentBuffer = g_gfxCurrentProducerBuffer;
    }

    return rc;
}

void gfxAppendFence(NvMultiFence* mf) {
    u32 max_fences = 4 - g_gfxQueueBufferData.fence.num_fences;
    u32 num_fences = max_fences < mf->num_fences ? max_fences : mf->num_fences;

    for (u32 i = 0; i < num_fences; i ++)
        g_gfxQueueBufferData.fence.fences[g_gfxQueueBufferData.fence.num_fences++] = mf->fences[i];
}

static Result _gfxQueueBuffer(void) {
    Result rc=0;

    if (g_gfxCurrentProducerBuffer >= 0) {
        rc = bqQueueBuffer(&g_gfxBinderSession, g_gfxCurrentProducerBuffer, &g_gfxQueueBufferData, &g_gfx_QueueBuffer_QueueBufferOutput);
        g_gfxQueueBufferData.fence.num_fences = 0;
        g_gfxCurrentProducerBuffer = -1;
    }

    return rc;
}

static Result _gfxCancelBuffer(void) {
    Result rc=0;

    if (g_gfxCurrentProducerBuffer >= 0) {
        rc = bqCancelBuffer(&g_gfxBinderSession, g_gfxCurrentProducerBuffer, &g_gfxQueueBufferData.fence);
        g_gfxQueueBufferData.fence.num_fences = 0;
        g_gfxCurrentProducerBuffer = -1;
    }

    return rc;
}

Result gfxInitDefault(void) {
    Result rc=0;

    if(g_gfxInitialized)return 0;

    g_gfxCurrentBuffer = -1;
    g_gfxCurrentProducerBuffer = -1;
    g_gfx_ProducerConnected = 0;
    g_gfxFramebuf = NULL;
    g_gfxFramebufSize = 0;
    g_gfxMode = GfxMode_LinearDouble;

    g_gfxQueueBufferData.transform = 0;

    //memset(g_gfx_ProducerSlotsRequested, 0, sizeof(g_gfx_ProducerSlotsRequested));
    g_gfx_ProducerSlotsRequested = 0;

    if (g_gfx_framebuf_width==0 || g_gfx_framebuf_height==0) {
        g_gfx_framebuf_width = 1280;
        g_gfx_framebuf_height = 720;
    }

    g_gfx_framebuf_display_width = g_gfx_framebuf_width;
    g_gfx_framebuf_display_height = g_gfx_framebuf_height;

    g_gfx_framebuf_aligned_width = (g_gfx_framebuf_width+15) & ~15;//Align to 16.
    g_gfx_framebuf_aligned_height = (g_gfx_framebuf_height+127) & ~127;//Align to 128.

    g_gfx_singleframebuf_size = g_gfx_framebuf_aligned_width*g_gfx_framebuf_aligned_height*4;
    g_gfx_singleframebuf_linear_size = g_gfx_framebuf_width*g_gfx_framebuf_height*4;

    g_gfx_BufferInitData.width = g_gfx_framebuf_width;
    g_gfx_BufferInitData.height = g_gfx_framebuf_height;
    g_gfx_BufferInitData.stride = g_gfx_framebuf_aligned_width;

    g_gfx_GraphicBuffer.stride = g_gfx_framebuf_aligned_width;
    g_gfx_GraphicBuffer.total_size = g_gfx_singleframebuf_size;

    g_gfx_GraphicBuffer.planes[0].width = g_gfx_framebuf_width;
    g_gfx_GraphicBuffer.planes[0].height = g_gfx_framebuf_height;
    g_gfx_GraphicBuffer.planes[0].pitch = g_gfx_framebuf_aligned_width*4;
    g_gfx_GraphicBuffer.planes[0].size = g_gfx_singleframebuf_size;

    g_gfxFramebufLinear = memalign(0x1000, g_gfx_singleframebuf_linear_size);
    if (g_gfxFramebufLinear) {
        memset(g_gfxFramebufLinear, 0, g_gfx_singleframebuf_linear_size);
    }
    else {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        return rc;
    }

    g_gfxFramebufSize = g_nvgfx_totalframebufs*g_gfx_singleframebuf_size;
    g_gfxFramebufSize = (g_gfxFramebufSize + 0xFFF) &~ 0xFFF; // page align
    g_gfxFramebuf = memalign(0x1000, g_gfxFramebufSize);
    if (!g_gfxFramebuf) {
        free(g_gfxFramebufLinear);
        g_gfxFramebufLinear = NULL;
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        return rc;
    }

    rc = viInitialize(__nx_gfx_vi_service_type);

    if (R_SUCCEEDED(rc)) rc = viOpenDefaultDisplay(&g_gfxDisplay);

    if (R_SUCCEEDED(rc)) rc = viGetDisplayVsyncEvent(&g_gfxDisplay, &g_gfxDisplayVsyncEvent);

    if (R_SUCCEEDED(rc)) rc = viCreateLayer(&g_gfxDisplay, &g_gfxLayer);

    if (R_SUCCEEDED(rc)) {
        binderCreate(&g_gfxBinderSession, g_gfxLayer.igbp_binder_obj_id);
        rc = binderInitSession(&g_gfxBinderSession);
    }

    if (R_SUCCEEDED(rc)) binderGetNativeHandle(&g_gfxBinderSession, 0x0f, &g_gfxBinderEvent); // a failure here is not fatal

    if (R_SUCCEEDED(rc)) rc = viSetLayerScalingMode(&g_gfxLayer, ViScalingMode_Default);

    if (R_SUCCEEDED(rc)) rc = bqConnect(&g_gfxBinderSession, NATIVE_WINDOW_API_CPU, 0, &g_gfx_Connect_QueueBufferOutput);

    if (R_SUCCEEDED(rc)) g_gfx_ProducerConnected = true;

    if (R_SUCCEEDED(rc)) rc = nvInitialize();

    if (R_SUCCEEDED(rc)) rc = nvFenceInit();

    if (R_SUCCEEDED(rc)) rc = nvMapInit();

    if (R_SUCCEEDED(rc)) rc = nvMapCreate(&g_nvmap_obj, g_gfxFramebuf, g_gfxFramebufSize, 0x20000, NvKind_Pitch, true);

    if (R_SUCCEEDED(rc)) {
        g_gfx_GraphicBuffer.nvmap_id = nvMapGetId(&g_nvmap_obj);
        for (s32 i = 0; i < g_nvgfx_totalframebufs; i ++) {
            g_gfx_GraphicBuffer.planes[0].offset = g_gfx_singleframebuf_size*i;
            rc = bqSetPreallocatedBuffer(&g_gfxBinderSession, i, &g_gfx_BufferInitData);
            if (R_FAILED(rc))
                break;
        }
    }

    if (R_SUCCEEDED(rc)) rc = _gfxDequeueBuffer();

    // todo: figure out if it's possible to wait for the 'Nintendo' logo screen to finish displaying (NSO, applet type: Application)

    if (R_FAILED(rc)) {
        if (g_gfx_ProducerConnected) {
            _gfxCancelBuffer();
            for(u32 i=0; i<32; i++) {
                if (g_gfx_ProducerSlotsRequested & BIT(i)) bqDetachBuffer(&g_gfxBinderSession, i);
            }
            bqDisconnect(&g_gfxBinderSession, NATIVE_WINDOW_API_CPU);
            nvMapClose(&g_nvmap_obj);
            nvMapExit();
            nvFenceExit();
            nvExit();
        }

        eventClose(&g_gfxBinderEvent);
        binderClose(&g_gfxBinderSession);
        viCloseLayer(&g_gfxLayer);
        eventClose(&g_gfxDisplayVsyncEvent);
        viCloseDisplay(&g_gfxDisplay);
        viExit();

        free(g_gfxFramebuf);
        free(g_gfxFramebufLinear);
        g_gfxFramebufLinear = NULL;

        g_gfxCurrentBuffer = 0;
        g_gfxCurrentProducerBuffer = -1;
        g_gfx_ProducerConnected = 0;
        g_gfxFramebuf = NULL;
        g_gfxFramebufSize = 0;

        g_gfx_framebuf_width = 0;
        g_gfx_framebuf_height = 0;

        g_gfx_ProducerSlotsRequested = 0;
    }

    if (R_SUCCEEDED(rc)) g_gfxInitialized = 1;

    return rc;
}

void gfxExit(void)
{
    if (!g_gfxInitialized)
        return;

    if (g_gfx_ProducerConnected) {
        _gfxCancelBuffer();
        for(u32 i=0; i<32; i++) {
            if (g_gfx_ProducerSlotsRequested & BIT(i)) bqDetachBuffer(&g_gfxBinderSession, i);
        }
        bqDisconnect(&g_gfxBinderSession, NATIVE_WINDOW_API_CPU);
        nvMapClose(&g_nvmap_obj);
        nvMapExit();
        nvFenceExit();
        nvExit();
    }

    eventClose(&g_gfxBinderEvent);
    binderClose(&g_gfxBinderSession);
    viCloseLayer(&g_gfxLayer);
    eventClose(&g_gfxDisplayVsyncEvent);
    viCloseDisplay(&g_gfxDisplay);
    viExit();

    free(g_gfxFramebuf);
    free(g_gfxFramebufLinear);
    g_gfxFramebufLinear = NULL;

    g_gfxInitialized = 0;

    g_gfxCurrentBuffer = 0;
    g_gfxCurrentProducerBuffer = -1;
    g_gfx_ProducerConnected = 0;
    g_gfxFramebuf = NULL;
    g_gfxFramebufSize = 0;

    g_gfx_framebuf_width = 0;
    g_gfx_framebuf_height = 0;

    gfxConfigureAutoResolution(0, 0, 0, 0, 0);

    g_gfx_ProducerSlotsRequested = 0;
    memset(&g_gfxQueueBufferData.crop, 0, sizeof(g_gfxQueueBufferData.crop));
}

void gfxInitResolution(u32 width, u32 height) {
    if (g_gfxInitialized) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_AlreadyInitialized));

    g_gfx_framebuf_width = (width+3) & ~3;
    g_gfx_framebuf_height = (height+3) & ~3;
}

void gfxInitResolutionDefault(void) {
    gfxInitResolution(1920, 1080);
}

void gfxConfigureCrop(s32 left, s32 top, s32 right, s32 bottom) {
    if (right==0 || bottom==0) {
        g_gfx_framebuf_display_width = g_gfx_framebuf_width;
        g_gfx_framebuf_display_height = g_gfx_framebuf_height;
    }

    if (left < 0 || top < 0 || right < 0 || bottom < 0) return;

    right = (right+3) & ~3;
    bottom = (bottom+3) & ~3;

    if (right < left || bottom < top) return;
    if (left > g_gfx_framebuf_width || top > g_gfx_framebuf_height) return;
    if (right > g_gfx_framebuf_width || bottom > g_gfx_framebuf_height) return;

    g_gfxQueueBufferData.crop.left = left;
    g_gfxQueueBufferData.crop.top = top;
    g_gfxQueueBufferData.crop.right = right;
    g_gfxQueueBufferData.crop.bottom = bottom;

    if (right!=0 && bottom!=0) {
        g_gfx_framebuf_display_width = right;
        g_gfx_framebuf_display_height = bottom;
    }
}

void gfxConfigureResolution(s32 width, s32 height) {
    gfxConfigureCrop(0, 0, width, height);
}

static void _gfxAutoResolutionAppletHook(AppletHookType hook, void* param) {
    u8 mode=0;

    if (hook != AppletHookType_OnOperationMode)
        return;

    mode = appletGetOperationMode();

    if (mode == AppletOperationMode_Handheld) {
        gfxConfigureResolution(g_gfx_autoresolution_handheld_width, g_gfx_autoresolution_handheld_height);
    }
    else if (mode == AppletOperationMode_Docked) {
        gfxConfigureResolution(g_gfx_autoresolution_docked_width, g_gfx_autoresolution_docked_height);
    }
}

void gfxConfigureAutoResolution(bool enable, s32 handheld_width, s32 handheld_height, s32 docked_width, s32 docked_height) {
    if (g_gfx_autoresolution_enabled != enable) {
        if(enable) appletHook(&g_gfx_autoresolution_applethookcookie, _gfxAutoResolutionAppletHook, 0);
        if (!enable) appletUnhook(&g_gfx_autoresolution_applethookcookie);
    }

    g_gfx_autoresolution_enabled = enable;

    g_gfx_autoresolution_handheld_width = handheld_width;
    g_gfx_autoresolution_handheld_height = handheld_height;
    g_gfx_autoresolution_docked_width = docked_width;
    g_gfx_autoresolution_docked_height = docked_height;

    if (enable) _gfxAutoResolutionAppletHook(AppletHookType_OnOperationMode, 0);
    if (!enable) gfxConfigureResolution(0, 0);
}

void gfxConfigureAutoResolutionDefault(bool enable) {
    gfxConfigureAutoResolution(enable, 1280, 720, 0, 0);
}

void gfxWaitForVsync(void) {
    Result rc = eventWait(&g_gfxDisplayVsyncEvent, U64_MAX);
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxEventWait));
}

void gfxSwapBuffers(void) {
    Result rc=0;

    rc = _gfxQueueBuffer();

    if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxQueueBuffer));

    rc = _gfxDequeueBuffer();

    if (R_FAILED(rc)) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGfxDequeueBuffer));
}

u32 gfxGetFramebufferHandle(u32 index, u32* offset) {
    if (offset) {
        index = (g_gfxCurrentBuffer + index) & (g_nvgfx_totalframebufs-1);
        *offset = index*g_gfx_singleframebuf_size;
    }
    return nvMapGetHandle(&g_nvmap_obj);
}

u8* gfxGetFramebuffer(u32* width, u32* height) {
    if(width) *width = g_gfx_framebuf_display_width;
    if(height) *height = g_gfx_framebuf_display_height;

    if (g_gfxMode == GfxMode_LinearDouble)
        return g_gfxFramebufLinear;

    return &g_gfxFramebuf[g_gfxCurrentBuffer*g_gfx_singleframebuf_size];
}

void gfxGetFramebufferResolution(u32* width, u32* height) {
    if(width) *width = g_gfx_framebuf_width;
    if(height) *height = g_gfx_framebuf_height;
}

size_t gfxGetFramebufferSize(void) {
    if (g_gfxMode == GfxMode_LinearDouble)
        return g_gfx_singleframebuf_linear_size;

    return g_gfx_singleframebuf_size;
}

u32 gfxGetFramebufferPitch(void) {
    return g_gfx_framebuf_aligned_width*4;
}

void gfxSetMode(GfxMode mode) {
    g_gfxMode = mode;
}

void gfxConfigureTransform(u32 transform) {
    g_gfxQueueBufferData.transform = transform;
}

void gfxFlushBuffers(void) {
    u32 *actual_framebuf = (u32*)&g_gfxFramebuf[g_gfxCurrentBuffer*g_gfx_singleframebuf_size];

    if (g_gfxMode == GfxMode_LinearDouble) {
        size_t x, y;
        size_t width = g_gfx_framebuf_display_width;
        size_t height = g_gfx_framebuf_display_height;
        u32 *in_framebuf = (u32*)g_gfxFramebufLinear;

        for (y=0; y<height; y++) {
            for (x=0; x<width; x+=4) {
                *((u128*)&actual_framebuf[gfxGetFramebufferDisplayOffset(x, y)]) = *((u128*)&in_framebuf[y * width + x]);
            }
        }
    }

    armDCacheFlush(actual_framebuf, g_gfx_singleframebuf_size);
}

/*static Result _gfxGetDisplayResolution(u64 *width, u64 *height) {
    return viGetDisplayResolution(&g_gfxDisplay, width, height);
}*/
