#include <string.h>
#include <malloc.h>
#include <switch.h>

static bool g_nvgfxInitialized = 0;
static u32 g_nvgfx_fd_nvhostctrlgpu;
static u32 g_nvgfx_fd_nvhostasgpu;
static u32 g_nvgfx_fd_nvmap;
static u32 g_nvgfx_nvmapobj;

static gpu_characteristics g_nvgfx_gpu_characteristics;
static u64 g_nvgfx_nvhostasgpu_allocspace_offset;

static u8 *g_nvgfx_nvmap_mem;
size_t g_nvgfx_nvmap_mem_size;

Result nvgfxInitialize(void) {
    Result rc=0;
    if(g_nvgfxInitialized)return 0;

    g_nvgfx_fd_nvhostctrlgpu = 0;
    g_nvgfx_fd_nvhostasgpu = 0;
    g_nvgfx_fd_nvmap = 0;
    g_nvgfx_nvmapobj = 0;

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(gpu_characteristics));
    g_nvgfx_nvhostasgpu_allocspace_offset = 0;

    g_nvgfx_nvmap_mem_size = 0x10000;

    if (R_SUCCEEDED(rc)) {
        g_nvgfx_nvmap_mem = memalign(0x1000, g_nvgfx_nvmap_mem_size);
        if (g_nvgfx_nvmap_mem==NULL) rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
    }

    //Officially NVHOST_IOCTL_CTRL_GET_CONFIG is used a lot (here and later), skip that.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);

    //Officially NVGPU_GPU_IOCTL_GET_TPC_MASKS is used here.

    //Officially NVGPU_GPU_IOCTL_ZCULL_GET_CTX_SIZE and NVGPU_GPU_IOCTL_ZCULL_GET_INFO are used here.

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvhostasgpu, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_InitializeEx(g_nvgfx_fd_nvhostasgpu, 1);

    //NVGPU_AS_IOCTL_GET_VA_REGIONS twice

    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_AllocSpace(g_nvgfx_fd_nvhostasgpu, 0x10000, 0x1000, 0, 0x1000, &g_nvgfx_nvhostasgpu_allocspace_offset);

    if (R_SUCCEEDED(rc)) rc = nvOpen(&g_nvgfx_fd_nvmap, "/dev/nvmap");

    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Create(g_nvgfx_fd_nvmap, g_nvgfx_nvmap_mem_size, &g_nvgfx_nvmapobj);

    if (R_SUCCEEDED(rc)) rc = nvioctlNvmap_Alloc(g_nvgfx_fd_nvmap, g_nvgfx_nvmapobj, 0, 0, 0x20000, 0, g_nvgfx_nvmap_mem);

    //Currently broken.
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0, g_nvgfx_nvmapobj, 0x1000, 0, g_nvgfx_nvmap_mem_size, NULL);
    if (R_SUCCEEDED(rc)) rc = -1;
    if (R_SUCCEEDED(rc)) rc = nvioctlNvhostAsGpu_MapBufferEx(g_nvgfx_fd_nvhostasgpu, 4, 0xfe, g_nvgfx_nvmapobj, 0x1000, 0, g_nvgfx_nvmap_mem_size, NULL);

    if (R_FAILED(rc)) nvClose(g_nvgfx_fd_nvmap);
    if (R_FAILED(rc)) nvClose(g_nvgfx_fd_nvhostasgpu);
    if (R_FAILED(rc)) nvClose(g_nvgfx_fd_nvhostctrlgpu);

    if (g_nvgfx_nvmap_mem) {
        free(g_nvgfx_nvmap_mem);
        g_nvgfx_nvmap_mem = NULL;
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

