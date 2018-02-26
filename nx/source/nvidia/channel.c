#include <switch.h>

Result nvchannelCreate(NvChannel* c, const char* dev)
{
    Result rc;

    c->has_init = true;

    rc = nvOpen(&c->fd, dev);

    if (R_FAILED(rc))
        c->fd = -1;

    if (R_FAILED(rc))
        nvchannelClose(c);

    return rc;
}

void nvchannelClose(NvChannel* c)
{
    if (!c->has_init)
        return;

    if (c->fd != -1)
        nvClose(c->fd);

    c->fd = -1;
}
