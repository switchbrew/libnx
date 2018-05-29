#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/buffer.h"
#include "nvidia/address_space.h"

static u32 g_nvmap_fd = -1;
static u64 g_refCnt;

Result nvBufferInit(void)
{
    Result rc;

    if (atomicIncrement64(&g_refCnt) > 0)
        return 0;

     rc = nvOpen(&g_nvmap_fd, "/dev/nvmap");

     if (R_FAILED(rc))
         atomicDecrement64(&g_refCnt);

     return rc;
}

void nvBufferExit(void)
{
    if (atomicDecrement64(&g_refCnt) == 0)
    {
        if (g_nvmap_fd != -1)
            nvClose(g_nvmap_fd);

        g_nvmap_fd = -1;
    }
}

u32 nvBufferGetNvmapFd(void) {
    return g_nvmap_fd;
}

static Result _nvBufferCreate(
    NvBuffer* m, size_t size, u32 flags, u32 align, NvBufferKind kind,
    NvAddressSpace* as)
{
    Result rc;

    size = (size + align - 1) & ~(align - 1);

    m->has_init = true;
    m->size = size;
    m->fd = -1;
    m->cpu_addr = memalign(align, size);
    m->gpu_addr = 0;
    m->gpu_addr_texture = 0;
    m->addr_space = as;
    m->kind = kind;

    if (m->cpu_addr == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    rc = nvioctlNvmap_Create(g_nvmap_fd, size, &m->fd);

    if (R_FAILED(rc))
        m->fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvmap_Alloc(
            g_nvmap_fd, m->fd, 0, flags | NvBufferFlags_Nintendo, align, kind, m->cpu_addr);

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceMapBuffer(as, m, 0, &m->gpu_addr);

    if (R_FAILED(rc))
        nvBufferFree(m);

    return rc;
}

Result nvBufferCreate(
        NvBuffer* m, size_t size, u32 align, NvBufferKind kind, NvAddressSpace* as) {
    return _nvBufferCreate(m, size, 0, align, kind, as);
}

Result nvBufferCreateRw(
        NvBuffer* m, size_t size, u32 align, NvBufferKind kind, NvAddressSpace* as) {
    return _nvBufferCreate(m, size, NvBufferFlags_Writable, align, kind, as);
}

Result nvBufferMakeCpuUncached(NvBuffer* m) {
    return svcSetMemoryAttribute(m->cpu_addr, m->size, 8, 8);
}

Result nvBufferMakeCpuCached(NvBuffer* m) {
    return svcSetMemoryAttribute(m->cpu_addr, m->size, 8, 0);
}

void nvBufferFree(NvBuffer* m)
{
    if (!m->has_init)
        return;

    // todo: nvAddressSpaceUnmapBuffer(m->gpu_addr)
    // todo: nvAddressSpaceUnmapBuffer(m->gpu_addr_texture)
    nvBufferMakeCpuCached(m);

    free(m->cpu_addr);
    m->cpu_addr = NULL;

    if (m->fd != -1)
        nvClose(m->fd);

    m->fd = -1;
}

void* nvBufferGetCpuAddr(NvBuffer* m) {
    return m->cpu_addr;
}

iova_t nvBufferGetGpuAddr(NvBuffer* m) {
    return m->gpu_addr;
}

Result nvBufferMapAsTexture(NvBuffer* m, NvBufferKind kind) {
    return nvAddressSpaceMapBuffer(m->addr_space, m, kind, &m->gpu_addr_texture);
}

iova_t nvBufferGetGpuAddrTexture(NvBuffer* m) {
    return m->gpu_addr_texture;
}
