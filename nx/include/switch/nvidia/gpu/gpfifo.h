#pragma once

typedef struct {
    NvChannel* parent;
} NvGpfifo;

Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent);
void   nvGpfifoClose(NvGpfifo* f);

#define NV_MAKE_GPFIFO_ENTRY(iova, size) \
    ((iova) | (((u64)(size)) << 42))

Result nvGpfifoSubmitCmdList(NvGpfifo* f, NvCmdList* cmd_list, u32 fence_incr, NvFence* fence_out);
