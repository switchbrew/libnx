#include <switch.h>

static u32 g_ctrlgpu_fd = -1;
static nvioctl_gpu_characteristics g_gpu_characteristics;
static u32 g_zcull_ctx_size;

Result nvinfoInit()
{
    Result rc;

    rc = nvOpen(&g_ctrlgpu_fd, "/dev/nvhost-ctrl-gpu");

    if (R_FAILED(rc))
        g_ctrlgpu_fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_ctrlgpu_fd, &g_gpu_characteristics);

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_ctrlgpu_fd, &g_zcull_ctx_size);

    if (R_FAILED(rc))
        nvinfoExit();

    return rc;
}

void nvinfoExit()
{
    if (g_ctrlgpu_fd != -1)
        nvClose(g_ctrlgpu_fd);

    g_ctrlgpu_fd = -1;
}

u32 nvinfoGetZcullCtxSize() {
    return g_zcull_ctx_size;
}
