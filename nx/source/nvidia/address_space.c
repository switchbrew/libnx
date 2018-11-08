#include <malloc.h>
#include "types.h"
#include "result.h"
#include "arm/atomics.h"
#include "kernel/svc.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "nvidia/address_space.h"

Result nvAddressSpaceCreate(NvAddressSpace* a, u32 page_size)
{
    Result rc;

    a->has_init = true;
    a->page_size = page_size;

    rc = nvOpen(&a->fd, "/dev/nvhost-as-gpu");

    if (R_FAILED(rc))
        a->fd = -1;

    if (R_SUCCEEDED(rc))
        rc = nvioctlNvhostAsGpu_InitializeEx(a->fd, 1, 0x10000); // Official sw uses hardcoded size

    if (R_FAILED(rc))
        nvAddressSpaceClose(a);

    return rc;
}

void nvAddressSpaceClose(NvAddressSpace* a)
{
    if (!a->has_init)
        return;

    if (a->fd != -1)
        nvClose(a->fd);

    a->fd = -1;
    a->has_init = false;
}

Result nvAddressSpaceAlloc(NvAddressSpace* a, bool sparse, u64 size, iova_t* iova_out)
{
    u32 num_pages = (size + a->page_size - 1) / a->page_size; // Round up to page size
    return nvioctlNvhostAsGpu_AllocSpace(a->fd, num_pages, a->page_size,
        sparse ? NvAllocSpaceFlags_Sparse : 0, a->page_size, iova_out);
}

Result nvAddressSpaceAllocFixed(NvAddressSpace* a, bool sparse, u64 size, iova_t iova)
{
    if (iova & (a->page_size - 1))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u32 num_pages = size / a->page_size; // Round down
    return nvioctlNvhostAsGpu_AllocSpace(a->fd, num_pages, a->page_size,
        NvMapBufferFlags_FixedOffset | (sparse ? NvAllocSpaceFlags_Sparse : 0), iova, NULL);
}

Result nvAddressSpaceFree(NvAddressSpace* a, iova_t iova, u64 size)
{
    if (iova & (a->page_size - 1))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u32 num_pages = size / a->page_size; // Round down
    return nvioctlNvhostAsGpu_FreeSpace(a->fd, iova, num_pages, a->page_size);
}

Result nvAddressSpaceMap(NvAddressSpace* a, u32 nvmap_handle, bool is_gpu_cacheable, NvKind kind, iova_t* iova_out)
{
    return nvioctlNvhostAsGpu_MapBufferEx(a->fd,
        is_gpu_cacheable ? NvMapBufferFlags_IsCacheable : 0, kind,
        nvmap_handle, a->page_size, 0, 0, 0, iova_out);
}

Result nvAddressSpaceMapFixed(NvAddressSpace* a, u32 nvmap_handle, bool is_gpu_cacheable, NvKind kind, iova_t iova)
{
    if (iova & (a->page_size - 1))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    return nvioctlNvhostAsGpu_MapBufferEx(a->fd,
        NvMapBufferFlags_FixedOffset | (is_gpu_cacheable ? NvMapBufferFlags_IsCacheable : 0), kind,
        nvmap_handle, a->page_size, 0, 0, iova, NULL);
}

Result nvAddressSpaceModify(NvAddressSpace* a, iova_t iova, u64 offset, u64 size, NvKind kind)
{
    if (iova & (a->page_size - 1))
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    u64 end_offset = (offset + size + a->page_size - 1) &~ (a->page_size - 1);
    offset &= a->page_size - 1;
    size = end_offset - offset;
    return nvioctlNvhostAsGpu_MapBufferEx(a->fd, NvMapBufferFlags_Modify, kind, 0, 0, offset, size, iova, NULL);
}

Result nvAddressSpaceUnmap(NvAddressSpace* a, iova_t iova)
{
    return nvioctlNvhostAsGpu_UnmapBuffer(a->fd, iova);
}
