#include <string.h>
#include <malloc.h>
#include <switch.h>

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

static gpu_characteristics g_nvgfx_gpu_characteristics;
static u64 g_nvgfx_nvhostasgpu_allocspace_offset;
static u32 g_nvgfx_tpcmasks[24>>2];
static u32 g_nvgfx_zcullctxsize;
static u32 g_nvgfx_zcullinfo[40>>2];
static nvioctl_va_region g_nvgfx_nvhostasgpu_varegions[2];
static nvioctl_l2_state g_nvgfx_l2state;
static nvioctl_fence g_nvgfx_nvhost_fence;
static u8 *g_nvgfx_nvhost_userdata;
static size_t g_nvgfx_nvhost_userdata_size;

static nvmapobj nvmap_objs[3];

Result nvmapobjInitialize(nvmapobj *obj, size_t size) {
    Result rc=0;

    if(obj->initialized)return 0;

    memset(obj, 0, sizeof(nvmapobj));

    obj->mem_size = size;

    obj->mem = memalign(0x1000, size);
    if (obj->mem==NULL) rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
    if (R_SUCCEEDED(rc)) memset(obj->mem, 0, size);

    if (R_SUCCEEDED(rc)) obj->initialized = 1;

    return rc;
}

void nvmapobjClose(nvmapobj *obj) {
    if(!obj->initialized)return;

    if (obj->mem) {
        free(obj->mem);
        obj->mem = NULL;
    }

    memset(obj, 0, sizeof(nvmapobj));
}

void nvmapobjCloseAll(void) {
    u32 pos=0;

    for(pos=0; pos<sizeof(nvmap_objs)/sizeof(nvmapobj); pos++) nvmapobjClose(&nvmap_objs[pos]);
}

Result nvmapobjSetup(nvmapobj *obj, u32 heapmask, u32 flags, u32 align, u8 kind) {
    Result rc=0;

    rc = nvioctlNvmap_Create(g_nvgfx_fd_nvmap, obj->mem_size, &obj->handle);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Alloc(g_nvgfx_fd_nvmap, obj->handle, heapmask, flags, align, kind, obj->mem);

    return rc;
}

Result nvgfxInitialize(void) {
    Result rc=0;
    if(g_nvgfxInitialized)return 0;

    g_nvgfx_fd_nvhostctrlgpu = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostgpu = 0;

    memset(nvmap_objs, 0, sizeof(nvmap_objs));

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(gpu_characteristics));
    memset(g_nvgfx_tpcmasks, 0, sizeof(g_nvgfx_tpcmasks));
    memset(g_nvgfx_zcullinfo, 0, sizeof(g_nvgfx_zcullinfo));
    memset(g_nvgfx_nvhostasgpu_varegions, 0, sizeof(g_nvgfx_nvhostasgpu_varegions));
    memset(&g_nvgfx_l2state, 0, sizeof(nvioctl_l2_state));
    memset(&g_nvgfx_nvhost_fence, 0, sizeof(g_nvgfx_nvhost_fence));
    g_nvgfx_nvhostasgpu_allocspace_offset = 0;
    g_nvgfx_zcullctxsize = 0;

    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[0], 0x1000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[1], 0x10000);
    if (R_SUCCEEDED(rc)) rc = nvmapobjInitialize(&nvmap_objs[2], 0x1000);

    if (R_SUCCEEDED(rc)) { //Unknown what size/etc is used officially.
        g_nvgfx_nvhost_userdata_size = 0x1000;
        g_nvgfx_nvhost_userdata = memalign(0x1000, g_nvgfx_nvhost_userdata_size);
        if (g_nvgfx_nvhost_userdata==NULL) rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
        if (R_SUCCEEDED(rc)) memset(g_nvgfx_nvhost_userdata, 0, g_nvgfx_nvhost_userdata_size);
    }

    //Officially NVHOST_IOCTL_CTRL_GET_CONFIG is used a lot (here and later), skip that.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetTpcMasks(g_nvgfx_fd_nvhostctrlgpu, 4, g_nvgfx_tpcmasks);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_zcullctxsize);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetInfo(g_nvgfx_fd_nvhostctrlgpu, g_nvgfx_zcullinfo);

    //Officially nvQueryEvent is used here.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostasgpu, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_InitializeEx(g_nvgfx_fd_nvhostasgpu, 1);

    //Officially this is used twice here - only use it once here.
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_GetVARegions(g_nvgfx_fd_nvhostasgpu, g_nvgfx_nvhostasgpu_varegions);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_AllocSpace(g_nvgfx_fd_nvhostasgpu, 0x10000, 0x20000, 0, 0x10000, &g_nvgfx_nvhostasgpu_allocspace_offset);
    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[0], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0, nvmap_objs[0].handle, 0x10000, 0, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0xfe, nvmap_objs[0].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[1], 0, 0, 0x20000, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 5, 0, nvmap_objs[1].handle, 0x10000, 0, 0x10000, g_nvgfx_nvhostasgpu_allocspace_offset, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, nvmap_objs[1].handle, 0x10000, 0, 0, 0, NULL);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetL2State(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_l2state);

    if (R_SUCCEEDED(rc)) rc = nvmapobjSetup(&nvmap_objs[2], 0, 0x1, 0x1000, 0);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostgpu, "/dev/nvhost-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_BindChannel(g_nvgfx_fd_nvhostasgpu, g_nvgfx_fd_nvhostgpu);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetNvmapFd(g_nvgfx_fd_nvhostgpu, g_nvgfx_fd_nvmap);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocGPFIFOEx2(g_nvgfx_fd_nvhostgpu, 0x800, 0x1, 0, 0, 0, 0, &g_nvgfx_nvhost_fence);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_AllocObjCtx(g_nvgfx_fd_nvhostgpu, NVIOCTL_CHANNEL_OBJ_CLASSNUM_3d, 0);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetErrorNotifier(g_nvgfx_fd_nvhostgpu, 0, 0x1000, nvmap_objs[2].handle);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetUserData(g_nvgfx_fd_nvhostgpu, g_nvgfx_nvhost_userdata);

    if (R_SUCCEEDED(rc)) rc = nvioctlChannel_SetPriority(g_nvgfx_fd_nvhostgpu, NVIOCTL_CHANNEL_PRIORITY_medium);

    //if (R_SUCCEEDED(rc)) rc = -1;

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

    nvClose(g_nvgfx_fd_nvhostgpu);
    nvClose(g_nvgfx_fd_nvmap);
    nvClose(g_nvgfx_fd_nvhostasgpu);
    nvClose(g_nvgfx_fd_nvhostctrlgpu);
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

