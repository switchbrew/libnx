#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/map.h"
#include "nvidia/channel.h"

Result nvChannelCreate(NvChannel* c, const char* dev)
{
    Result rc;

    c->has_init = true;

    rc = nvOpen(&c->fd, dev);

    if (R_FAILED(rc))
        c->fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlChannel_SetNvmapFd(c->fd, nvMapGetFd());

    if (R_FAILED(rc))
        nvChannelClose(c);

    return rc;
}

void nvChannelClose(NvChannel* c)
{
    if (!c->has_init)
        return;

    if (c->fd != -1)
        nvClose(c->fd);

    c->fd = -1;
}

Result nvChannelSetPriority(NvChannel* c, NvChannelPriority prio) {
    return nvioctlChannel_SetPriority(c->fd, prio);
}

Result nvChannelSetTimeout(NvChannel* c, u32 timeout) {
    return nvioctlChannel_SetTimeout(c->fd, timeout);
}
