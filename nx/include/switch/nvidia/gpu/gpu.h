#pragma once

typedef struct {
    NvAddressSpace addr_space;
    NvChannel gpu_channel;
} NvGpu;

Result nvgpuCreate(NvGpu* g);
void   nvgpuClose(NvGpu* g);
