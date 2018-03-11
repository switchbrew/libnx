#pragma once

typedef struct {
    NvChannel* parent;
    nvioctl_fence fifo_fence;
} NvGpfifo;

Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent);
void   nvGpfifoClose(NvGpfifo* f);

#define NV_MAKE_GPFIFO_ENTRY(iova, size) \
    ((iova) | (((u64)(size)) << 42))

Result nvGpfifoSubmit(NvGpfifo* f, NvCmdList* cmd_list, NvFence* fence_out);
