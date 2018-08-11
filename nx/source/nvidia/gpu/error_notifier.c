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
#include "nvidia/gpu/cmd_list.h"
#include "nvidia/gpu/gpfifo.h"
#include "nvidia/gpu/zcull_ctx.h"
#include "nvidia/gpu/3d_ctx.h"
#include "nvidia/gpu/error_notifier.h"
#include "nvidia/gpu/gpu.h"

Result nvErrorNotifierCreate(NvErrorNotifier* t, NvGpu* parent)
{
    Result rc;
    Handle handle;

    rc = nvQueryEvent(
        parent->gpu_channel.fd, NvEventId_Gpu_ErrorNotifier, &handle);

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(&t->event, handle);
        rc = nvioctlChannel_SetErrorNotifier(parent->gpu_channel.fd, 1);
    }

    if (R_SUCCEEDED(rc)) {
        t->parent = parent;
        t->has_init = true;
    }

    return rc;
}

void nvErrorNotifierClose(NvErrorNotifier* t)
{
    if (!t->has_init)
        return;

    nvioctlChannel_SetErrorNotifier(t->parent->gpu_channel.fd, 0);
    eventClose(&t->event);
}

Result nvErrorNotifierWait(NvErrorNotifier* t, u64 timeout) {
    return eventWait(&t->event, timeout);
}

Result nvErrorNotifierGetError(NvErrorNotifier* t, NvError* out) {
    return nvioctlChannel_GetErrorNotification(t->parent->gpu_channel.fd, out);
}
