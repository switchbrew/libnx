#pragma once
#include "../kernel/event.h"
#include "channel.h"
#include "fence.h"

#define GPFIFO_QUEUE_SIZE        0x800
#define GPFIFO_ENTRY_NOT_MAIN    BIT(9)
#define GPFIFO_ENTRY_NO_PREFETCH BIT(31)

typedef struct NvGpuChannel
{
    NvChannel base;
    Event error_event;
    u64 object_id;
    NvFence fence;
    u32 fence_incr;
    nvioctl_gpfifo_entry entries[GPFIFO_QUEUE_SIZE];
    u32 num_entries;
} NvGpuChannel;

Result nvGpuChannelCreate(NvGpuChannel* c, struct NvAddressSpace* as);
void   nvGpuChannelClose(NvGpuChannel* c);

Result nvGpuChannelZcullBind(NvGpuChannel* c, iova_t iova);
Result nvGpuChannelAppendEntry(NvGpuChannel* c, iova_t start, size_t num_cmds, u32 flags, u32 flush_threshold);
Result nvGpuChannelKickoff(NvGpuChannel* c);
Result nvGpuChannelGetErrorNotification(NvGpuChannel* c, NvError* error);

static inline u32 nvGpuChannelGetSyncpointId(NvGpuChannel* c)
{
    return c->fence.id;
}

static inline void nvGpuChannelGetFence(NvGpuChannel* c, NvFence* fence_out)
{
    fence_out->id = c->fence.id;
    fence_out->value = c->fence.value + c->fence_incr;
}

static inline void nvGpuChannelIncrFence(NvGpuChannel* c)
{
    ++c->fence_incr;
}
