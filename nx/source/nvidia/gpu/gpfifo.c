#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "kernel/event.h"
#include "kernel/detect.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/buffer.h"
#include "nvidia/address_space.h"
#include "nvidia/channel.h"
#include "nvidia/fence.h"
#include "nvidia/info.h"
#include "nvidia/gpu/cmd_list.h"
#include "nvidia/gpu/gpfifo.h"
#include "nvidia/gpu/zcull_ctx.h"
#include "nvidia/gpu/3d_ctx.h"
#include "nvidia/gpu/error_notifier.h"
#include "nvidia/gpu/gpu.h"


Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent)
{
    f->parent = parent;

    NvFence fence;
    Result res = nvioctlChannel_AllocGpfifoEx2(parent->fd, GPFIFO_QUEUE_SIZE, 1, 0, 0, 0, 0, &fence);
    f->syncpt_id = fence.id;
    return res;
}

void nvGpfifoClose(NvGpfifo* f) {
    /**/
}

Result nvGpfifoAppendEntry(NvGpfifo* f, iova_t start, size_t num_cmds, u32 flags)
{
    if (f->num_entries >= GPFIFO_QUEUE_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    nvioctl_gpfifo_entry* entry = &f->entries[f->num_entries++];
    entry->desc = start;
    entry->desc32[1] |= flags | (num_cmds << 10);
    return 0;
}

Result nvGpfifoAppendCmdList(NvGpfifo* f, NvCmdList* cmd_list, u32 flags)
{
    Result rc = nvGpfifoAppendEntry(f,
        nvCmdListGetGpuAddr(cmd_list) + 4*cmd_list->offset,
        cmd_list->num_cmds,
        flags);

    if (R_SUCCEEDED(rc)) {
        cmd_list->offset += cmd_list->num_cmds;
        cmd_list->num_cmds = 0;
    }

    return rc;
}

Result nvGpfifoFlush(NvGpfifo* f, u32 fence_incr, NvFence* fence_out)
{
    Result rc;
    NvFence fence;

    if (!f->num_entries)
        return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    fence.id = 0;
    fence.value = fence_incr;

    u32 flags = BIT(2);
    if (fence_incr)
        flags |= BIT(8);

    if (kernelAbove400())
        rc = nvioctlChannel_KickoffPb(f->parent->fd, f->entries, f->num_entries, flags, &fence);
    else
        rc = nvioctlChannel_SubmitGpfifo(f->parent->fd, f->entries, f->num_entries, flags, &fence);

    if (R_SUCCEEDED(rc)) {
        f->num_entries = 0;
        if (fence_out)
            *fence_out = fence;
    }

    return rc;
}
