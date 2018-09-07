#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "kernel/event.h"
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

#define DEFAULT_FIFO_ENTRIES 0x800

Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent)
{
    f->parent = parent;

    NvFence fence;
    Result res = nvioctlChannel_AllocGpfifoEx2(parent->fd, DEFAULT_FIFO_ENTRIES, 1, 0, 0, 0, 0, &fence);
    //__builtin_printf("nvGpfifoCreate initial fence: %d %u\n", (int)fence.id, fence.value);
    //if (R_SUCCEEDED(res) && (s32)fence.id >= 0)
    //    nvFenceWait(&fence, -1);
    return res;
}

void nvGpfifoClose(NvGpfifo* f) {
    /**/
}

Result nvGpfifoSubmitCmdList(NvGpfifo* f, NvCmdList* cmd_list, u32 fence_incr, NvFence* fence_out)
{
    Result rc;
    nvioctl_gpfifo_entry ent;
    NvFence fence;

    ent.desc = nvCmdListGetGpuAddr(cmd_list) + 4*cmd_list->offset;
    ent.desc32[1] |= (2 << 8) | (nvCmdListGetListSize(cmd_list) << 10);

    fence.id = 0;
    fence.value = fence_incr;

    u32 flags = BIT(2);
    if (fence_incr)
        flags |= BIT(8);

    rc = nvioctlChannel_SubmitGpfifo(
        f->parent->fd, &ent, 1, flags, &fence);

    if (R_SUCCEEDED(rc) && fence_out)
        *fence_out = fence;

    cmd_list->offset += cmd_list->num_cmds;
    cmd_list->num_cmds = 0;

    return rc;
}
