#pragma once

typedef struct {
    NvChannel* parent;
    nvioctl_fence fifo_fence;
} NvGpfifo;

Result nvfifoCreate(NvGpfifo* f, NvChannel* parent);
void   nvfifoClose(NvGpfifo* f);

#define NV_MAKE_GPFIFO_ENTRY(iova, size) \
    ((iova) | (((u64)(size)) << 42))

Result nvfifoSubmit(NvGpfifo* f, NvCmdList* cmd_list, NvFence* fence_out);
