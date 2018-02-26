#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/buffer.h"

static u32 g_nvmap_fd;

Result nvbufInit() {
    return nvOpen(&g_nvmap_fd, "/dev/nvmap");
}

Result nvbufExit() {
    return nvClose(g_nvmap_fd);
}

static Result _nvbufCreate(NvBuffer* m, size_t size, u32 flags, u32 align, NvBufferKind kind)
{
    Result rc;

    m->has_init = true;
    m->size = size;
    m->fd = -1;
    m->ptr = memalign(size, align);

    if (m->ptr == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    rc = nvioctlNvmap_Create(g_nvmap_fd, size, &m->fd);

    if (R_FAILED(rc))
        m->fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvmap_Alloc(g_nvmap_fd, m->fd, 0, flags, align, kind, m->ptr);

    if (R_FAILED(rc))
        nvbufFree(m);

    return rc;
}

Result nvbufCreate(NvBuffer* m, size_t size, u32 align, NvBufferKind kind) {
    return _nvbufCreate(m, size, 0, align, kind);
}

Result nvbufCreateRw(NvBuffer* m, size_t size, u32 align, NvBufferKind kind) {
    return _nvbufCreate(m, size, NvBufferFlags_Writable, align, kind);
}

void nvbufFree(NvBuffer* m)
{
    if (!m->has_init)
        return;

    free(m->ptr);
    m->ptr = NULL;

    if (m->fd != -1)
        nvClose(m->fd);

    m->fd = -1;
}

void* nvbufGetAddr(NvBuffer* m) {
    return m->ptr;
}
