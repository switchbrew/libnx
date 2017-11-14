#include <string.h>
#include <malloc.h>
#include <switch.h>

static bool g_nvgfxInitialized = 0;
static u32 g_nvgfx_fd_nvhostctrlgpu;
static u32 g_nvgfx_fd_nvhostasgpu;
static u32 g_nvgfx_fd_nvmap;
static u32 g_nvgfx_nvmapobj0;

static gpu_characteristics g_nvgfx_gpu_characteristics;
static u64 g_nvgfx_nvhostasgpu_allocspace_offset;
static u32 g_nvgfx_tpcmasks[24>>2];
static u32 g_nvgfx_zcullctxsize;
static u32 g_nvgfx_zcullinfo[40>>2];
static nvioctl_va_region g_nvgfx_nvhostasgpu_varegions[2];

static u8 *g_nvgfx_nvmap_mem;
size_t g_nvgfx_nvmap_mem_size;

Result nvgfxInitialize(void) {
    Result rc=0;
    if(g_nvgfxInitialized)return 0;

    g_nvgfx_fd_nvhostctrlgpu = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_nvmapobj0 = 0;

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(gpu_characteristics));
    memset(g_nvgfx_tpcmasks, 0, sizeof(g_nvgfx_tpcmasks));
    memset(g_nvgfx_zcullinfo, 0, sizeof(g_nvgfx_zcullinfo));
    memset(g_nvgfx_nvhostasgpu_varegions, 0, sizeof(g_nvgfx_nvhostasgpu_varegions));
    g_nvgfx_nvhostasgpu_allocspace_offset = 0;
    g_nvgfx_zcullctxsize = 0;

    g_nvgfx_nvmap_mem_size = 0x1000;

    if (R_SUCCEEDED(rc)) {
        g_nvgfx_nvmap_mem = memalign(0x1000, g_nvgfx_nvmap_mem_size);
        if (g_nvgfx_nvmap_mem==NULL) rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
        if (R_SUCCEEDED(rc)) memset(g_nvgfx_nvmap_mem, 0, g_nvgfx_nvmap_mem_size);
    }

    //Officially NVHOST_IOCTL_CTRL_GET_CONFIG is used a lot (here and later), skip that.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetTpcMasks(g_nvgfx_fd_nvhostctrlgpu, 4, g_nvgfx_tpcmasks);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_zcullctxsize);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_ZCullGetInfo(g_nvgfx_fd_nvhostctrlgpu, g_nvgfx_zcullinfo);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostasgpu, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_InitializeEx(g_nvgfx_fd_nvhostasgpu, 1);

    //Officially this is used twice here - only use it once here.
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_GetVARegions(g_nvgfx_fd_nvhostasgpu, g_nvgfx_nvhostasgpu_varegions);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_AllocSpace(g_nvgfx_fd_nvhostasgpu, 0x10000, 0x20000, 0, 0x10000, &g_nvgfx_nvhostasgpu_allocspace_offset);
    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Create(g_nvgfx_fd_nvmap, g_nvgfx_nvmap_mem_size, &g_nvgfx_nvmapobj0);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Alloc(g_nvgfx_fd_nvmap, g_nvgfx_nvmapobj0, 0, 0, 0x20000, 0, g_nvgfx_nvmap_mem);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0, g_nvgfx_nvmapobj0, 0x10000, 0, 0, NULL);
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 0, 0xfe, g_nvgfx_nvmapobj0, 0x10000, 0, 0, NULL);
    //if (R_SUCCEEDED(rc)) rc = -1;

    if (R_FAILED(rc)) {
        nvClose(g_nvgfx_fd_nvmap);
        nvClose(g_nvgfx_fd_nvhostasgpu);
        nvClose(g_nvgfx_fd_nvhostctrlgpu);

        if (g_nvgfx_nvmap_mem) {
            free(g_nvgfx_nvmap_mem);
            g_nvgfx_nvmap_mem = NULL;
        }
    }

    if (R_SUCCEEDED(rc)) g_nvgfxInitialized = 1;

    return rc;
}

void nvgfxExit(void) {
    if(!g_nvgfxInitialized)return;

    nvClose(g_nvgfx_fd_nvmap);
    nvClose(g_nvgfx_fd_nvhostasgpu);
    nvClose(g_nvgfx_fd_nvhostctrlgpu);
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvhostctrlgpu = 0;

    if (g_nvgfx_nvmap_mem) {
        free(g_nvgfx_nvmap_mem);
        g_nvgfx_nvmap_mem = NULL;
    }

    g_nvgfxInitialized = 0;
}

