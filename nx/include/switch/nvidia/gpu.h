#pragma once
#include "types.h"
#include "ioctl.h"

Result nvGpuInit(void);
void nvGpuExit(void);

const nvioctl_gpu_characteristics* nvGpuGetCharacteristics(void);
u32 nvGpuGetZcullCtxSize(void);
const nvioctl_zcull_info* nvGpuGetZcullInfo(void);
const u32* nvGpuGetTpcMasks(u32 *num_masks_out);

Result nvGpuZbcGetActiveSlotMask(u32 *out_slot, u32 *out_mask);
Result nvGpuZbcAddColor(const u32 color_l2[4], const u32 color_ds[4], u32 format);
Result nvGpuZbcAddDepth(float depth);
