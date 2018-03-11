#include <switch.h>

Result nvChannelCreate(NvChannel* c, const char* dev)
{
    Result rc;

    c->has_init = true;

    rc = nvOpen(&c->fd, dev);

    if (R_FAILED(rc))
        c->fd = -1;

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

Result nvChannelSetNvmapFd(NvChannel* c) {
    return nvioctlChannel_SetNvmapFd(c->fd, nvBufferGetNvmapFd());
}
