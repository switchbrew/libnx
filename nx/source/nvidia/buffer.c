#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "arm/cache.h"
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

Result nvBufferCreate(
    NvBuffer* m, size_t size, u32 align, bool is_cpu_cacheable, bool is_gpu_cacheable, NvKind kind,
    NvAddressSpace* as)
{
    Result rc;

    size = (size + align - 1) & ~(align - 1);

    m->has_init = true;
    m->is_cpu_cacheable = is_cpu_cacheable;
    m->is_gpu_cacheable = is_gpu_cacheable;
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

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvmap_Alloc(g_nvmap_fd, m->fd,
            0, is_cpu_cacheable ? 1 : 0, align, kind, m->cpu_addr);

    if (R_SUCCEEDED(rc) && !is_cpu_cacheable) {
        armDCacheFlush(m->cpu_addr, m->size);
        svcSetMemoryAttribute(m->cpu_addr, m->size, 8, 8);
    }

    if (R_SUCCEEDED(rc))
        rc = nvAddressSpaceMap(as, m->fd, is_gpu_cacheable, NvKind_Pitch, &m->gpu_addr);

    if (R_FAILED(rc))
        nvBufferFree(m);

    return rc;
}

void nvBufferFree(NvBuffer* m)
{
    if (!m->has_init)
        return;

    if (m->gpu_addr_texture) {
        nvAddressSpaceUnmap(m->addr_space, m->gpu_addr_texture);
        m->gpu_addr_texture = 0;
    }

    if (m->gpu_addr) {
        nvAddressSpaceUnmap(m->addr_space, m->gpu_addr);
        m->gpu_addr = 0;
    }

    if (m->fd != -1) {
        nvioctlNvmap_Free(g_nvmap_fd, m->fd);
        m->fd = -1;
    }

    if (m->cpu_addr) {
        if (!m->is_cpu_cacheable)
            svcSetMemoryAttribute(m->cpu_addr, m->size, 8, 0);

        free(m->cpu_addr);
        m->cpu_addr = NULL;
    }

    m->has_init = false;
}

void* nvBufferGetCpuAddr(NvBuffer* m) {
    return m->cpu_addr;
}

iova_t nvBufferGetGpuAddr(NvBuffer* m) {
    return m->gpu_addr;
}

Result nvBufferMapAsTexture(NvBuffer* m, NvKind kind) {
    return nvAddressSpaceMap(m->addr_space, m->fd, m->is_gpu_cacheable, kind, &m->gpu_addr_texture);
}

iova_t nvBufferGetGpuAddrTexture(NvBuffer* m) {
    return m->gpu_addr_texture;
}
