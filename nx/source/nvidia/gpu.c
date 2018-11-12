#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/gpu.h"

static u32 g_ctrlgpu_fd = -1;
static u64 g_refCnt;

static nvioctl_gpu_characteristics g_gpu_characteristics;
static u32 g_zcull_ctx_size;

Result nvGpuInit(void)
{
    Result rc;

    if (atomicIncrement64(&g_refCnt) > 0)
        return 0;

    rc = nvOpen(&g_ctrlgpu_fd, "/dev/nvhost-ctrl-gpu");

    if (R_FAILED(rc))
        g_ctrlgpu_fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_GetCharacteristics(g_ctrlgpu_fd, &g_gpu_characteristics);

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_ZCullGetCtxSize(g_ctrlgpu_fd, &g_zcull_ctx_size);

    if (R_FAILED(rc))
        nvGpuExit();

    return rc;
}

void nvGpuExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0) {
        if (g_ctrlgpu_fd != -1)
            nvClose(g_ctrlgpu_fd);

        g_ctrlgpu_fd = -1;
    }
}

const nvioctl_gpu_characteristics* nvGpuGetCharacteristics(void)
{
    return &g_gpu_characteristics;
}

u32 nvGpuGetZcullCtxSize(void)
{
    return g_zcull_ctx_size;
}
