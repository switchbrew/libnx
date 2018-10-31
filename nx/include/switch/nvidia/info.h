#pragma once
#include "types.h"
#include "ioctl.h"

Result nvInfoInit(void);
void nvInfoExit(void);

const nvioctl_gpu_characteristics* nvInfoGetGpuCharacteristics(void);
u32 nvInfoGetZcullCtxSize(void);
