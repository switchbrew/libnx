#pragma once

#define GPFIFO_QUEUE_SIZE        0x800
#define GPFIFO_ENTRY_NOT_MAIN    BIT(9)
#define GPFIFO_ENTRY_NO_PREFETCH BIT(31)

typedef struct {
    NvChannel* parent;
    u32 syncpt_id;
    u32 num_entries;
    nvioctl_gpfifo_entry entries[GPFIFO_QUEUE_SIZE];
} NvGpfifo;

Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent);
void   nvGpfifoClose(NvGpfifo* f);

Result nvGpfifoAppendEntry(NvGpfifo* f, iova_t start, size_t num_cmds, u32 flags);
Result nvGpfifoAppendCmdList(NvGpfifo* f, NvCmdList* cmd_list, u32 flags);
Result nvGpfifoFlush(NvGpfifo* f, u32 fence_incr, NvFence* fence_out);
