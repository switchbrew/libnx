#pragma once

typedef struct NvGpu {
    NvAddressSpace addr_space;
    NvChannel gpu_channel;
    NvGpfifo gpfifo;
    NvZcullContext zcull_ctx;
    Nv3DContext _3d_ctx;
    NvErrorNotifier error_notifier;
} NvGpu;

Result nvGpuCreate(NvGpu* g);
void   nvGpuClose(NvGpu* g);
