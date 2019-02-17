#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "runtime/hosversion.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/map.h"
#include "nvidia/address_space.h"
#include "nvidia/fence.h"
#include "nvidia/gpu_channel.h"

Result nvGpuChannelCreate(NvGpuChannel* c, struct NvAddressSpace* as)
{
    Result res;

    res = nvChannelCreate(&c->base, "/dev/nvhost-gpu");
    if (R_FAILED(res))
        return res;

    c->fence_incr = 0;
    c->num_entries = 0;

    res = nvioctlNvhostAsGpu_BindChannel(as->fd, c->base.fd);

    if (R_SUCCEEDED(res))
        res = nvioctlChannel_AllocGpfifoEx2(c->base.fd, GPFIFO_QUEUE_SIZE, 1, 0, 0, 0, 0, &c->fence);

    if (R_SUCCEEDED(res))
        res = nvioctlChannel_AllocObjCtx(c->base.fd, NvClassNumber_3D, 0, &c->object_id);

    if (R_SUCCEEDED(res))
        res = nvQueryEvent(c->base.fd, NvEventId_Gpu_ErrorNotifier, &c->error_event);

    if (R_SUCCEEDED(res))
        res = nvioctlChannel_SetErrorNotifier(c->base.fd, 1);

    if (R_SUCCEEDED(res))
        res = nvChannelSetPriority(&c->base, NvChannelPriority_Medium);

    if (R_FAILED(res))
        nvGpuChannelClose(c);

    return res;
}

void nvGpuChannelClose(NvGpuChannel* c)
{
    if (!c->base.has_init)
        return;

    if (c->error_event.revent != INVALID_HANDLE) {
        nvioctlChannel_SetErrorNotifier(c->base.fd, 0);
        eventClose(&c->error_event);
    }

    nvChannelClose(&c->base);
}

Result nvGpuChannelZcullBind(NvGpuChannel* c, iova_t iova)
{
    return nvioctlChannel_ZCullBind(c->base.fd, iova, NvZcullConfig_SeparateBuffer);
}

Result nvGpuChannelAppendEntry(NvGpuChannel* c, iova_t start, size_t num_cmds, u32 flags, u32 flush_threshold)
{
    Result res;
    if (flush_threshold >= GPFIFO_QUEUE_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (c->num_entries >= (GPFIFO_QUEUE_SIZE-flush_threshold)) {
        res = nvGpuChannelKickoff(c);
        if (R_FAILED(res))
            return res;
    }
    if (c->num_entries >= GPFIFO_QUEUE_SIZE)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    nvioctl_gpfifo_entry* entry = &c->entries[c->num_entries++];
    entry->desc = start;
    entry->desc32[1] |= flags | (num_cmds << 10);
    return 0;
}

static Result _nvGpuChannelKickoffRaw(NvGpuChannel* c, u32 flags)
{
    NvFence fence;
    fence.id = 0;
    fence.value = c->fence_incr;

    if (hosversionAtLeast(4,0,0))
        return nvioctlChannel_KickoffPb(c->base.fd, c->entries, c->num_entries, flags, &fence);
    else
        return nvioctlChannel_SubmitGpfifo(c->base.fd, c->entries, c->num_entries, flags, &fence);
}

Result nvGpuChannelKickoff(NvGpuChannel* c)
{
    if (!c->num_entries)
        return 0;

    u32 flags = BIT(2);
    if (c->fence_incr)
        flags |= BIT(8);

    Result res;
    for (;;)
    {
        res = _nvGpuChannelKickoffRaw(c, flags);
        if (res != MAKERESULT(Module_LibnxNvidia, LibnxNvidiaError_Busy))
            break;
        svcSleepThread(100000000);
    }

    if (R_SUCCEEDED(res)) {
        c->fence.value += c->fence_incr;
        c->fence_incr = 0;
        c->num_entries = 0;
    }

    return res;
}

Result nvGpuChannelGetErrorNotification(NvGpuChannel* c, NvError* error)
{
    Result res = eventWait(&c->error_event, 0);
    if (R_SUCCEEDED(res))
        res = nvioctlChannel_GetErrorNotification(c->base.fd, error);
    return res;
}
