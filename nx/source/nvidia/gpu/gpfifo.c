#include <switch.h>

#define DEFAULT_FIFO_ENTRIES 0x800

Result nvfifoCreate(NvGpfifo* f, NvChannel* parent)
{
    f->parent = parent;

    return nvioctlChannel_AllocGpfifoEx2(
        parent->fd, DEFAULT_FIFO_ENTRIES, 1, 0, 0, 0, 0, &f->fifo_fence);
}

void nvfifoClose() {
    /**/
}
