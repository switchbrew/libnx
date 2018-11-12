#pragma once
#include "types.h"
#include "ioctl.h"

Result nvGpuInit(void);
void nvGpuExit(void);

const nvioctl_gpu_characteristics* nvGpuGetCharacteristics(void);
u32 nvGpuGetZcullCtxSize(void);
