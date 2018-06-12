#include <switch.h>

#define DEFAULT_FIFO_ENTRIES 0x800

Result nvGpfifoCreate(NvGpfifo* f, NvChannel* parent)
{
    f->parent = parent;

    return nvioctlChannel_AllocGpfifoEx2(
        parent->fd, DEFAULT_FIFO_ENTRIES, 1, 0, 0, 0, 0, &f->fifo_fence);
}

void nvGpfifoClose(NvGpfifo* f) {
    /**/
}

Result nvGpfifoSubmit(NvGpfifo* f, NvCmdList* cmd_list, NvFence* fence_out)
{
    Result rc;
    nvioctl_gpfifo_entry ent;
    nvioctl_fence fence;

    u64 a =
        nvCmdListGetGpuAddr(cmd_list) | (nvCmdListGetListSize(cmd_list) << 42);

    ent.desc32[0] = a;
    ent.desc32[1] = a >> 32;

    fence.id = 0;
    fence.value = 1;

    rc = nvioctlChannel_SubmitGpfifo(
        f->parent->fd, &ent, 1, /*0x104*/0x104/*flags*/, &fence);

    if (R_SUCCEEDED(rc)) {
        nvfenceCreate(fence_out, &fence);
    }

    cmd_list->num_cmds = 0;

    return rc;
}
