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

    ent.desc = nvCmdListGetGpuAddr(cmd_list) | (nvCmdListGetListSize(cmd_list) << 42);

    rc = nvioctlChannel_SubmitGpfifo(
        f->parent->fd, &ent, 1, 0/*flags*/, &fence);

    if (R_SUCCEEDED(rc)) {
        nvfenceCreate(fence_out, &fence);
    }

    return rc;
}
