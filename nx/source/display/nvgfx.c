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

static bool g_nvgfxInitialized = 0;
static u32 g_nvgfx_fd_nvhostctrlgpu;
static u32 g_nvgfx_fd_nvhostasgpu;
static u32 g_nvgfx_fd_nvmap;
static u32 g_nvgfx_fd_nvhostgpu;

static nvioctl_gpu_characteristics g_nvgfx_gpu_characteristics;
static u64 g_nvgfx_nvhostasgpu_allocspace_offset;
static u32 g_nvgfx_zcullctxsize;
static u32 g_nvgfx_zcullinfo[40>>2];
static nvioctl_fence g_nvgfx_nvhost_fence;

u32 g_nvgfx_totalframebufs = 0;

static nvmapobj nvmap_objs[18];

static u64 nvmap_obj4_mapbuffer_x0_offset;
static u64 nvmap_obj6_mapbuffer_xdb_offset;

static u64 g_nvgfx_gpfifo_pos = 0;

extern size_t g_gfx_singleframebuf_size;

Result _gfxGraphicBufferInit(s32 buf, u32 nvmap_handle);

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

static void nvmapobjCloseAll(void) {
    u32 pos=0;

    for(pos=0; pos<sizeof(nvmap_objs)/sizeof(nvmapobj); pos++) nvmapobjClose(&nvmap_objs[pos]);
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

    u32 framebuf_nvmap_handle = 0;//Special handle ID for framebuf/windowbuf.

    g_nvgfx_fd_nvhostctrlgpu = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostgpu = 0;

    g_nvgfx_totalframebufs = 2;

    memset(nvmap_objs, 0, sizeof(nvmap_objs));

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(nvioctl_gpu_characteristics));
    memset(g_nvgfx_zcullinfo, 0, sizeof(g_nvgfx_zcullinfo));
    memset(&g_nvgfx_nvhost_fence, 0, sizeof(g_nvgfx_nvhost_fence));
    g_nvgfx_nvhostasgpu_allocspace_offset = 0;
    g_nvgfx_zcullctxsize = 0;
    nvmap_obj4_mapbuffer_x0_offset = 0;
    nvmap_obj6_mapbuffer_xdb_offset = 0;

    g_nvgfx_gpfifo_pos = 0;

    //All of the below sizes for nvmapobjInitialize are from certain official sw.
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[4], 0x59000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[6], g_nvgfx_totalframebufs*g_gfx_singleframebuf_size);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_zcullctxsize);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetInfo(g_nvgfx_fd_nvhostctrlgpu, g_nvgfx_zcullinfo);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostasgpu, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_InitializeEx(g_nvgfx_fd_nvhostasgpu, 1, /*0*/0x10000);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_AllocSpace(g_nvgfx_fd_nvhostasgpu, 0x10000, /*0x20000*/0x10000, 0, 0x10000, &g_nvgfx_nvhostasgpu_allocspace_offset);
    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");
    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostgpu, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_BindChannel(g_nvgfx_fd_nvhostasgpu, g_nvgfx_fd_nvhostgpu);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetNvmapFd(g_nvgfx_fd_nvhostgpu, g_nvgfx_fd_nvmap);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocGpfifoEx2(g_nvgfx_fd_nvhostgpu, 0x800, 0x1, 0, 0, 0, 0, &g_nvgfx_nvhost_fence);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocObjCtx(g_nvgfx_fd_nvhostgpu, g_nvgfx_gpu_characteristics.threed_class, 0);
    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetPriority(g_nvgfx_fd_nvhostgpu, NvChannelPriority_Medium);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[4], 0, 0x1, 0x20000, 0);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[4].handle, 0x10000, 0, 0, 0, &nvmap_obj4_mapbuffer_x0_offset);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[4].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_ZCullBind(g_nvgfx_fd_nvhostgpu, nvmap_obj4_mapbuffer_x0_offset+0x8000, 0x2);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[6], 0, 0x1, 0x20000, 0);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[6].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[6].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xdb, nvmap_objs[6].handle, 0x10000, 0, 0, 0, &nvmap_obj6_mapbuffer_xdb_offset);

    if (R_SUCCEEDED(rc))
    {
        u32 pos;
        for (pos=0; pos<g_nvgfx_totalframebufs; pos++) {
            rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0x100, 0xdb, framebuf_nvmap_handle, 0, pos*g_gfx_singleframebuf_size, g_gfx_singleframebuf_size, nvmap_obj6_mapbuffer_xdb_offset, NULL);
            if (R_FAILED(rc)) break;
        }
    }

    if (R_SUCCEEDED(rc))
    {
        u32 i;
        for (i=0; i<2; i++)
        {
            u32 new_id = 0;
            rc = nvioctlNvmap_GetId(g_nvgfx_fd_nvmap, nvmap_objs[6].handle, &new_id);

            if (R_FAILED(rc))
                break;

            if (new_id == -1) {
                rc = 6; // todo: Convert this error
                break;
            }

            u32 new_nvmap_fd;
            rc = nvioctlNvmap_FromId(g_nvgfx_fd_nvmap, new_id, &new_nvmap_fd);

            if (R_FAILED(rc))
                break;

            rc = _gfxGraphicBufferInit(i, new_nvmap_fd);

            if (R_FAILED(rc))
                break;
        }
    }

    if (R_FAILED(rc)) {
        nvClose(g_nvgfx_fd_nvhostgpu);
        nvClose(g_nvgfx_fd_nvmap);
        nvClose(g_nvgfx_fd_nvhostasgpu);
        nvClose(g_nvgfx_fd_nvhostctrlgpu);
        g_nvgfx_fd_nvhostgpu = 0;
        g_nvgfx_fd_nvmap = 0;
        g_nvgfx_fd_nvhostasgpu = 0;
        g_nvgfx_fd_nvhostctrlgpu = 0;

        nvmapobjCloseAll();
    }

    if (R_SUCCEEDED(rc))
        g_nvgfxInitialized = true;

    return rc;
}

void nvgfxExit(void) {
    if (!g_nvgfxInitialized)
        return;

    nvClose(g_nvgfx_fd_nvhostgpu);
    nvClose(g_nvgfx_fd_nvmap);
    nvClose(g_nvgfx_fd_nvhostasgpu);
    nvClose(g_nvgfx_fd_nvhostctrlgpu);
    g_nvgfx_fd_nvhostgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvhostctrlgpu = 0;

    nvmapobjCloseAll();

    g_nvgfxInitialized = false;
}

Result nvgfxEventWait(u32 syncpt_id, u32 threshold, s32 timeout)
{
    Result rc;

    do {
        int event_res;
        rc = nvioctlNvhostCtrl_EventWait(g_nvgfx_fd_nvhostctrl, syncpt_id, threshold, timeout, 0, &event_res);
    } while (rc == MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Timeout)); // todo: Fix timeout error

    return rc;
}

Result nvgfxGetFramebuffer(u8 **buffer, size_t *size)
{
    if (!g_nvgfxInitialized)
        return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if (buffer != NULL)
        *buffer = nvmap_objs[6].mem;
    if (size != NULL)
        *size = nvmap_objs[6].mem_size;

    return 0;
}

