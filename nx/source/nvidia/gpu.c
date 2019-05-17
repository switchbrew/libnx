#include <string.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/gpu.h"

// Official software hardcodes this for Tegra X1 (and even K1).
#define NUM_TPC_MASKS 1

static u32 g_ctrlgpu_fd = -1;
static u64 g_refCnt;

static nvioctl_gpu_characteristics g_gpu_characteristics;
static u32 g_zcull_ctx_size;
static nvioctl_zcull_info g_zcull_info;
static u32 g_tpc_masks[NUM_TPC_MASKS];

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

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_ZCullGetInfo(g_ctrlgpu_fd, &g_zcull_info);

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostCtrlGpu_GetTpcMasks(g_ctrlgpu_fd, g_tpc_masks, sizeof(g_tpc_masks));

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

const nvioctl_zcull_info* nvGpuGetZcullInfo(void)
{
    return &g_zcull_info;
}

const u32* nvGpuGetTpcMasks(u32 *num_masks_out)
{
    if (num_masks_out) *num_masks_out = NUM_TPC_MASKS;
    return g_tpc_masks;
}

Result nvGpuZbcGetActiveSlotMask(u32 *out_slot, u32 *out_mask)
{
    nvioctl_zbc_slot_mask data;
    Result rc = nvioctlNvhostCtrlGpu_ZbcGetActiveSlotMask(g_ctrlgpu_fd, &data);
    if (R_SUCCEEDED(rc)) {
        if (out_slot) *out_slot = data.slot;
        if (out_mask) *out_mask = data.mask;
    }
    return rc;
}

Result nvGpuZbcAddColor(const u32 color_l2[4], const u32 color_ds[4], u32 format)
{
    return nvioctlNvhostCtrlGpu_ZbcSetTable(g_ctrlgpu_fd, color_ds, color_l2, 0, format, NVGPU_ZBC_TYPE_COLOR);
}

Result nvGpuZbcAddDepth(float depth)
{
    u32 depth_int;
    memcpy(&depth_int, &depth, sizeof(float));
    return nvioctlNvhostCtrlGpu_ZbcSetTable(g_ctrlgpu_fd, NULL, NULL, depth_int, 1, NVGPU_ZBC_TYPE_DEPTH);
}
