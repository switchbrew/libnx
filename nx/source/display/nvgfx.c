#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "display/binder.h"
#include "display/buffer_producer.h"
#include "display/nvgfx.h"
#include "nvidia/ioctl.h"

typedef struct {
    bool initialized;
    u32 handle;
    u8 *mem;
    size_t mem_size;
} nvmapobj;

static bool g_nvgfxInitialized;
static u32 g_nvgfx_fd_nvmap;

u32 g_nvgfx_totalframebufs = 2;

static nvmapobj nvmap_fb_obj;

extern size_t g_gfx_singleframebuf_size;

Result _gfxGraphicBufferInit(s32 buf, u32 nvmap_id, u32 nvmap_handle);

static Result nvmapobjInitialize(nvmapobj *obj, size_t size) {
    Result rc=0;

    if(obj->initialized)return 0;

    memset(obj, 0, sizeof(nvmapobj));

    obj->mem_size = size;

    obj->mem = memalign(0x1000, size);
    if (obj->mem==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    if (R_SUCCEEDED(rc)) memset(obj->mem, 0, size);

    if (R_SUCCEEDED(rc)) armDCacheFlush(obj->mem, size);

    if (R_SUCCEEDED(rc)) obj->initialized = 1;

    return rc;
}

static void nvmapobjClose(nvmapobj *obj) {
    if(!obj->initialized)return;

    if (obj->mem) {
        free(obj->mem);
        obj->mem = NULL;
    }

    memset(obj, 0, sizeof(nvmapobj));
}

static Result nvmapobjSetup(nvmapobj *obj, u32 heapmask, u32 flags, u32 align, u8 kind) {
    Result rc=0;

    rc = nvioctlNvmap_Create(g_nvgfx_fd_nvmap, obj->mem_size, &obj->handle);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Alloc(g_nvgfx_fd_nvmap, obj->handle, heapmask, flags, align, kind, obj->mem);

    return rc;
}

Result nvgfxInitialize(void) {
    Result rc=0;

    if (g_nvgfxInitialized)
        return 0;

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_fb_obj, g_nvgfx_totalframebufs*g_gfx_singleframebuf_size);
    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_fb_obj, 0, 0x1, 0x20000, 0);

    if (R_SUCCEEDED(rc))
    {
        u32 id = 0;
        rc = nvioctlNvmap_GetId(g_nvgfx_fd_nvmap, nvmap_fb_obj.handle, &id);
        if (R_SUCCEEDED(rc))
        {
            for (int i = 0; i < g_nvgfx_totalframebufs; i ++)
            {
                rc = _gfxGraphicBufferInit(i, id, nvmap_fb_obj.handle);
                if (R_FAILED(rc))
                    break;
            }
        }
    }

    if (R_FAILED(rc)) {
        nvmapobjClose(&nvmap_fb_obj);
        if (g_nvgfx_fd_nvmap != -1) {
            nvClose(g_nvgfx_fd_nvmap);
            g_nvgfx_fd_nvmap = -1;
        }
    }

    if (R_SUCCEEDED(rc))
        g_nvgfxInitialized = true;

    return rc;
}

void nvgfxExit(void) {
    if (!g_nvgfxInitialized)
        return;

    nvmapobjClose(&nvmap_fb_obj);
    if (g_nvgfx_fd_nvmap != -1) {
        nvClose(g_nvgfx_fd_nvmap);
        g_nvgfx_fd_nvmap = -1;
    }

    g_nvgfxInitialized = false;
}

Result nvgfxGetFramebuffer(u8 **buffer, size_t *size, u32 *handle)
{
    if (!g_nvgfxInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer != NULL)
        *buffer = nvmap_fb_obj.mem;
    if (size != NULL)
        *size = nvmap_fb_obj.mem_size;
    if (handle != NULL)
        *handle = nvmap_fb_obj.handle;

    return 0;
}
