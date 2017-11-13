#include <string.h>
#include <switch.h>

static bool g_nvgfxInitialized = 0;
static u32 g_nvgfx_fd_nvhostctrlgpu;

static gpu_characteristics g_nvgfx_gpu_characteristics;

Result nvgfxInitialize(void) {
    Result rc=0;
    if(g_nvgfxInitialized)return 0;

    g_nvgfx_fd_nvhostctrlgpu = 0;

    memset(&g_nvgfx_gpu_characteristics, 0, sizeof(gpu_characteristics));

    //Officially NVHOST_IOCTL_CTRL_GET_CONFIG is used a lot (here and later), skip that.

    rc = nvOpen(&g_nvgfx_fd_nvhostctrlgpu, "/dev/nvhost-ctrl-gpu");
    if (R_SUCCEEDED(rc)) {
        rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_nvgfx_fd_nvhostctrlgpu, &g_nvgfx_gpu_characteristics);
        //TODO: What is the above output officially used for?
    }

    if (R_FAILED(rc)) nvClose(g_nvgfx_fd_nvhostctrlgpu);

    if (R_SUCCEEDED(rc)) g_nvgfxInitialized = 1;

    return rc;
}

void nvgfxExit(void) {
    if(!g_nvgfxInitialized)return;

    nvClose(g_nvgfx_fd_nvhostctrlgpu);
    g_nvgfx_fd_nvhostctrlgpu = 0;

    g_nvgfxInitialized = 0;
}

