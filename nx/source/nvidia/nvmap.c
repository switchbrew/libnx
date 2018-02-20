#include <malloc.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/nvmap.h"

static u32 g_nvmap_fd;

Result nvmapInit() {
    return nvOpen(&g_nvmap_fd, "/dev/nvmap");
}

Result nvmapExit() {
    return nvClose(g_nvmap_fd);
}

static Result _nvmapCreate(Nvmap* m, size_t size, u32 flags, u32 align, NvmapKind kind)
{
    Result rc;

    m->size = size;
    m->fd = -1;
    m->ptr = memalign(size, align);

    if (m->ptr == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    rc = nvioctlNvmap_Create(g_nvmap_fd, size, &m->fd);

    if (R_SUCCEEDED(rc)) {
        rc = nvioctlNvmap_Alloc(g_nvmap_fd, m->fd, 0, flags, align, kind, m->ptr);

        if (R_FAILED(rc)) {
            nvClose(m->fd);
            m->fd = -1;
        }
    }

    if (R_FAILED(rc)) {
        free(m->ptr);
        m->ptr = NULL;
    }

    return rc;
}

Result nvmapCreate(Nvmap* m, size_t size, u32 align, NvmapKind kind) {
    return _nvmapCreate(m, size, 0, align, kind);
}

Result nvmapCreateRw(Nvmap* m, size_t size, u32 align, NvmapKind kind) {
    return _nvmapCreate(m, size, NvmapFlags_Writable, align, kind);
}

void nvmapFree(Nvmap* m)
{
    free(m->ptr);
    m->ptr = NULL;

    nvClose(m->fd);
    m->fd = -1;
}

void* nvmapGetAddr(Nvmap* m) {
    return m->ptr;
}
