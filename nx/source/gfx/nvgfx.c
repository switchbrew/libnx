#include <string.h>
#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/cache.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "gfx/binder.h"
#include "gfx/buffer_producer.h"
#include "gfx/nvioctl.h"
#include "gfx/nvgfx.h"

//#include "nvgfx_gpu_gpfifo_data0_bin.h"

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
static u32 g_nvgfx_fd_nvhostctrl;

static Handle g_nvgfx_nvhostctrl_eventhandle = INVALID_HANDLE;
static Handle g_nvgfx_nvhostctrlgpu_event2 = INVALID_HANDLE;
static Handle g_nvgfx_nvhostgpu_event3 = INVALID_HANDLE;

static gpu_characteristics g_nvgfx_gpu_characteristics;
static u64 g_nvgfx_nvhostasgpu_allocspace_offset;
static u32 g_nvgfx_tpcmasks[24>>2];
static u32 g_nvgfx_zcullctxsize;
static u32 g_nvgfx_zcullinfo[40>>2];
static nvioctl_va_region g_nvgfx_nvhostasgpu_varegions[2];
static nvioctl_l2_state g_nvgfx_l2state;
static nvioctl_fence g_nvgfx_nvhost_fence;
nvioctl_fence g_nvgfx_nvhostgpu_gpfifo_fence;
static u8 *g_nvgfx_nvhost_userdata;
static size_t g_nvgfx_nvhost_userdata_size;
static u32 g_nvgfx_nvhostctrl_eventres;

u32 g_nvgfx_totalframebufs = 0;

static nvmapobj nvmap_objs[18];

static u64 nvmap_obj3_mapbuffer_x0_offset;
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
    u32 pos=0, i=0;
    s32 tmp=0;
    u32 tmpval=0;
    if(g_nvgfxInitialized)return 0;

    u32 framebuf_nvmap_handle = 0;//Special handle ID for framebuf/windowbuf.

    g_nvgfx_fd_nvhostctrlgpu = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostgpu = 0;
    g_nvgfx_fd_nvhostctrl = 0;

    g_nvgfx_nvhostctrl_eventhandle = INVALID_HANDLE;

    g_nvgfx_totalframebufs = 2;

    memset(nvmap_objs, 0, sizeof(nvmap_objs));

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(gpu_characteristics));
    memset(g_nvgfx_tpcmasks, 0, sizeof(g_nvgfx_tpcmasks));
    memset(g_nvgfx_zcullinfo, 0, sizeof(g_nvgfx_zcullinfo));
    memset(g_nvgfx_nvhostasgpu_varegions, 0, sizeof(g_nvgfx_nvhostasgpu_varegions));
    memset(&g_nvgfx_l2state, 0, sizeof(nvioctl_l2_state));
    memset(&g_nvgfx_nvhost_fence, 0, sizeof(g_nvgfx_nvhost_fence));
    memset(&g_nvgfx_nvhostgpu_gpfifo_fence, 0, sizeof(g_nvgfx_nvhostgpu_gpfifo_fence));
    g_nvgfx_nvhostasgpu_allocspace_offset = 0;
    g_nvgfx_zcullctxsize = 0;
    nvmap_obj3_mapbuffer_x0_offset = 0;
    nvmap_obj4_mapbuffer_x0_offset = 0;
    nvmap_obj6_mapbuffer_xdb_offset = 0;
    g_nvgfx_nvhostctrl_eventres = 0;

    g_nvgfx_gpfifo_pos = 0;

    //All of the below sizes for nvmapobjInitialize are from certain official sw.
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[0], 0x1000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[1], 0x10000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[2], 0x1000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[3], 0x10000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[4], 0x59000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[5], 0x1000000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[6], g_nvgfx_totalframebufs*g_gfx_singleframebuf_size);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[7], 0x1000000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[8], 0x800000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[9], 0x100000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[10], 0x3000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[11], 0x1000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[12], 0x1000);
    //if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[13], 0x1000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[14], 0x1000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[15], 0x6000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[16], 0x1000);

    if (R_SUCCEEDED(rc)) { //Unknown what size/etc is used officially.
        g_nvgfx_nvhost_userdata_size = 0x1000;
        g_nvgfx_nvhost_userdata = memalign(0x1000, g_nvgfx_nvhost_userdata_size);
        if (g_nvgfx_nvhost_userdata==NULL) rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        if (R_SUCCEEDED(rc)) memset(g_nvgfx_nvhost_userdata, 0, g_nvgfx_nvhost_userdata_size);
    }

    //Officially NVHOST_IOCTL_CTRL_GET_CONFIG is used a lot (here and later), skip that. This is done with a /dev/nvhost-ctrl fd, seperate from the one used later.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetTpcMasks(g_nvgfx_fd_nvhostctrlgpu, 4, g_nvgfx_tpcmasks);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_zcullctxsize);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetInfo(g_nvgfx_fd_nvhostctrlgpu, g_nvgfx_zcullinfo);

    if (R_SUCCEEDED(rc)) rc = nvQueryEvent(g_nvgfx_fd_nvhostctrlgpu, 2, &g_nvgfx_nvhostctrlgpu_event2);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostasgpu, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_InitializeEx(g_nvgfx_fd_nvhostasgpu, 1, /*0*/0x10000);

    //Officially this is used twice here - only use it once here.
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_GetVARegions(g_nvgfx_fd_nvhostasgpu, g_nvgfx_nvhostasgpu_varegions);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_AllocSpace(g_nvgfx_fd_nvhostasgpu, 0x10000, /*0x20000*/0x10000, 0, 0x10000, &g_nvgfx_nvhostasgpu_allocspace_offset);
    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[0], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0, nvmap_objs[0].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0xfe, nvmap_objs[0].handle, 0x10000, 0, 0, 0, NULL);*/

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[1], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[1].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[1].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetL2State(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_l2state);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[2], 0, 0x1, 0x1000, 0);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostgpu, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_BindChannel(g_nvgfx_fd_nvhostasgpu, g_nvgfx_fd_nvhostgpu);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetNvmapFd(g_nvgfx_fd_nvhostgpu, g_nvgfx_fd_nvmap);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocGpfifoEx2(g_nvgfx_fd_nvhostgpu, 0x800, 0x1, 0, 0, 0, 0, &g_nvgfx_nvhost_fence);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocObjCtx(g_nvgfx_fd_nvhostgpu, g_nvgfx_gpu_characteristics.threed_class, 0);

    if (R_SUCCEEDED(rc)) rc = nvQueryEvent(g_nvgfx_fd_nvhostgpu, 3, &g_nvgfx_nvhostgpu_event3);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetErrorNotifier(g_nvgfx_fd_nvhostgpu, 0, 0x1000, nvmap_objs[2].handle);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetUserData(g_nvgfx_fd_nvhostgpu, g_nvgfx_nvhost_userdata);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetPriority(g_nvgfx_fd_nvhostgpu, NvChannelPriority_Medium);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[3], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0, nvmap_objs[3].handle, 0x10000, 0, 0, 0, &nvmap_obj3_mapbuffer_x0_offset);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0xfe, nvmap_objs[3].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[4], 0, 0x1, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[4].handle, 0x10000, 0, 0, 0, &nvmap_obj4_mapbuffer_x0_offset);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[4].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_ZCullBind(g_nvgfx_fd_nvhostgpu, nvmap_obj4_mapbuffer_x0_offset+0x8000, 0x2);

    //Officially, ipcQueryPointerBufferSize and NVGPU_IOCTL_CHANNEL_SUBMIT_GPFIFO(nvioctlChannel_SubmitGpfifo) are used here with the duplicate service session setup during nv serv init.
    //This is used for GPU rendering.
    //if (R_SUCCEEDED(rc)) rc = nvgfxSubmitGpfifo();

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[5], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0, nvmap_objs[5].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0xfe, nvmap_objs[5].handle, 0x10000, 0, 0, 0, NULL);*/

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[6], 0, 0x1, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[6].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[6].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xdb, nvmap_objs[6].handle, 0x10000, 0, 0, 0, &nvmap_obj6_mapbuffer_xdb_offset);

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[7], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[7].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[7].handle, 0x10000, 0, 0, 0, NULL);*/

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[8], 0, 0x1, 0x20000, 0);

    //Currently broken.
    //if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[8].handle, 0x10000, 0, 0x800000, g_nvgfx_nvhostasgpu_allocspace_offset+0x10000, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[8].handle, 0x10000, 0, 0, 0, NULL);*/

    //Skip init for 0x10000000-byte nvmap obj done by certain official sw.

    if (R_SUCCEEDED(rc)) {
         for(pos=0; pos<g_nvgfx_totalframebufs; pos++) {
             rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0x100, 0xdb, framebuf_nvmap_handle, 0, pos*g_gfx_singleframebuf_size, g_gfx_singleframebuf_size, nvmap_obj6_mapbuffer_xdb_offset, NULL);
             if (R_FAILED(rc)) break;
         }
    }

    if (R_SUCCEEDED(rc)) {
        rc = bqQuery(NATIVE_WINDOW_FORMAT, &tmp);//TODO: What does official sw use the output from this for?

        if (R_SUCCEEDED(rc)) {
            for(i=0; i<2; i++) {
                tmpval = 0;
                rc = nvioctlNvmap_GetId(g_nvgfx_fd_nvmap, nvmap_objs[6].handle, &tmpval);
                if (R_FAILED(rc)) break;

                if(tmpval==~0) {
                    rc = 6;//official error
                    break;
                }

                rc = nvioctlNvmap_FromId(g_nvgfx_fd_nvmap, tmpval, &tmpval);
                if (R_FAILED(rc)) break;

                //The above gets a nvmap_handle, but normally it's the same value passed to nvioctlNvmap_GetId().

                rc = _gfxGraphicBufferInit(i, tmpval);
                if (R_FAILED(rc)) break;
            }
        }
    }

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[9], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[9].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[9].handle, 0x10000, 0, 0, 0, NULL);*/

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[10], 0, 0x1, 0x20000, 0);

    //Currently broken.
    //if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[10].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset+0x10000+0x800000, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[10].handle, 0x10000, 0, 0, 0, NULL);

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[11], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[11].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[11].handle, 0x10000, 0, 0, 0, NULL);*/

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[12], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[12].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[12].handle, 0x10000, 0, 0, 0, NULL);*/

    /*if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[13], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, nvmap_objs[13].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[13].handle, 0x10000, 0, 0, 0, NULL);*/

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[14], 0, 0x1, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[14].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset+0x10000+0x800000+0x10000, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[14].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[15], 0, 0x1, 0x20000, 0);

    //Currently broken.
    //if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[15].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset+0x10000+0x800000+0x10000+0x10000, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[15].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[16], 0, 0x1, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[16].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset+0x10000+0x800000+0x10000+0x10000+0x10000, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[16].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrl, "/dev/nvhost-ctrl");

    //if (R_SUCCEEDED(rc)) rc = -1;

    if (R_FAILED(rc)) {
        if (g_nvgfx_nvhostctrl_eventhandle != INVALID_HANDLE) {
            svcCloseHandle(g_nvgfx_nvhostctrl_eventhandle);
            g_nvgfx_nvhostctrl_eventhandle = INVALID_HANDLE;
       }

       if (g_nvgfx_nvhostgpu_event3 != INVALID_HANDLE) {
           svcCloseHandle(g_nvgfx_nvhostgpu_event3);
           g_nvgfx_nvhostgpu_event3 = INVALID_HANDLE;
       }

       if (g_nvgfx_nvhostctrlgpu_event2 != INVALID_HANDLE) {
           svcCloseHandle(g_nvgfx_nvhostctrlgpu_event2);
           g_nvgfx_nvhostctrlgpu_event2 = INVALID_HANDLE;
       }

        nvClose(g_nvgfx_fd_nvhostctrl);
        nvClose(g_nvgfx_fd_nvhostgpu);
        nvClose(g_nvgfx_fd_nvmap);
        nvClose(g_nvgfx_fd_nvhostasgpu);
        nvClose(g_nvgfx_fd_nvhostctrlgpu);
        g_nvgfx_fd_nvhostctrl = 0;
        g_nvgfx_fd_nvhostgpu = 0;
        g_nvgfx_fd_nvmap = 0;
        g_nvgfx_fd_nvhostasgpu = 0;
        g_nvgfx_fd_nvhostctrlgpu = 0;

        nvmapobjCloseAll();

        if(g_nvgfx_nvhost_userdata) {
            free(g_nvgfx_nvhost_userdata);
            g_nvgfx_nvhost_userdata = NULL;
        }
    }

    if (R_SUCCEEDED(rc)) g_nvgfxInitialized = 1;

    return rc;
}

void nvgfxExit(void) {
    if(!g_nvgfxInitialized)return;

    if (g_nvgfx_nvhostctrl_eventhandle != INVALID_HANDLE) {
        svcCloseHandle(g_nvgfx_nvhostctrl_eventhandle);
        g_nvgfx_nvhostctrl_eventhandle = INVALID_HANDLE;
    }

    if (g_nvgfx_nvhostgpu_event3 != INVALID_HANDLE) {
        svcCloseHandle(g_nvgfx_nvhostgpu_event3);
        g_nvgfx_nvhostgpu_event3 = INVALID_HANDLE;
    }

    if (g_nvgfx_nvhostctrlgpu_event2 != INVALID_HANDLE) {
        svcCloseHandle(g_nvgfx_nvhostctrlgpu_event2);
        g_nvgfx_nvhostctrlgpu_event2 = INVALID_HANDLE;
    }

    nvClose(g_nvgfx_fd_nvhostctrl);
    nvClose(g_nvgfx_fd_nvhostgpu);
    nvClose(g_nvgfx_fd_nvmap);
    nvClose(g_nvgfx_fd_nvhostasgpu);
    nvClose(g_nvgfx_fd_nvhostctrlgpu);
    g_nvgfx_fd_nvhostctrl = 0;
    g_nvgfx_fd_nvhostgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvhostctrlgpu = 0;

    nvmapobjCloseAll();

    if(g_nvgfx_nvhost_userdata) {
        free(g_nvgfx_nvhost_userdata);
        g_nvgfx_nvhost_userdata = NULL;
    }

    g_nvgfxInitialized = 0;
}

Result nvgfxEventWait(u32 syncpt_id, u32 threshold, s32 timeout) {
    Result rc=0;
    Result timeout_rc = MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Timeout);

    if (R_SUCCEEDED(rc)) {
        do {
            rc = nvioctlNvhostCtrl_EventWait(g_nvgfx_fd_nvhostctrl, syncpt_id, threshold, timeout, 0, &g_nvgfx_nvhostctrl_eventres);
        } while(rc==timeout_rc);
    }

    //Official sw only uses the below block when event-waiting timeout occurs.
    //Currently broken.
    //if (R_SUCCEEDED(rc)) rc = nvQueryEvent(g_nvgfx_fd_nvhostctrl, g_nvgfx_nvhostctrl_eventres, &g_nvgfx_nvhostctrl_eventhandle);

    /*if (R_SUCCEEDED(rc)) {
        svcCloseHandle(g_nvgfx_nvhostctrl_eventhandle);
        g_nvgfx_nvhostctrl_eventhandle = INVALID_HANDLE;
    }*/

    //if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrl_EventSignal(g_nvgfx_fd_nvhostctrl, g_nvgfx_nvhostctrl_eventres);

    return rc;
}

Result nvgfxSubmitGpfifo(void) {
    //Extracted from memory of certain official sw.
    u8 gpfifo_data[] = {0x00, 0x00, 0x00, 0x00, 0x51, 0x04, 0x00, 0x80, 0xB2, 0x00, 0x01, 0x20, 0x42, 0x00, 0x10, 0x00, 0x51, 0x04, 0x00, 0x80, 0xC0, 0x06, 0x04, 0x20, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0xF0, 0x00, 0x10, 0xE0, 0x03, 0x00, 0x80};

    nvioctl_gpfifo_entry gpfifo_entries[2];

    u64 total_size = 0x1778+sizeof(gpfifo_data);
    u64 tmpva;

    if (g_nvgfx_gpfifo_pos >= nvmap_objs[3].mem_size || g_nvgfx_gpfifo_pos+total_size >= nvmap_objs[3].mem_size) g_nvgfx_gpfifo_pos = 0;

    //memcpy(&nvmap_objs[3].mem[g_nvgfx_gpfifo_pos], nvgfx_gpu_gpfifo_data0_bin, nvgfx_gpu_gpfifo_data0_bin_size);
    memcpy(&nvmap_objs[3].mem[g_nvgfx_gpfifo_pos+0x1778], gpfifo_data, sizeof(gpfifo_data));
    armDCacheFlush(nvmap_objs[3].mem, nvmap_objs[3].mem_size);

    tmpva = nvmap_obj3_mapbuffer_x0_offset+g_nvgfx_gpfifo_pos;
    gpfifo_entries[0].entry0 = (u32)tmpva | (0x00177aULL<<40);
    gpfifo_entries[0].entry1 = ((u32)(tmpva>>32)) | (0x00177a<<8);

    tmpva = nvmap_obj3_mapbuffer_x0_offset+g_nvgfx_gpfifo_pos+0x1778;
    gpfifo_entries[1].entry0 = (u32)tmpva;
    gpfifo_entries[1].entry1 = ((u32)(tmpva>>32)) | (0x80002e<<8);

    g_nvgfx_gpfifo_pos+= total_size;

    Result rc = nvioctlChannel_SubmitGpfifo(g_nvgfx_fd_nvhostgpu, gpfifo_entries, 2, 0x104, &g_nvgfx_nvhostgpu_gpfifo_fence);

    return rc;
}

Result nvgfxGetFramebuffer(u8 **buffer, size_t *size) {
    if(!g_nvgfxInitialized)return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

    if(buffer) *buffer = nvmap_objs[6].mem;
    if(size) *size = nvmap_objs[6].mem_size;

    return 0;
}

