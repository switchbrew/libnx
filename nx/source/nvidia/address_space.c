#include <switch.h>

Result nvasCreate(NvAddressSpace* a)
{
    Result rc = nvOpen(&a->fd, "/dev/nvhost-as-gpu");

    if (R_SUCCEEDED(rc))
    {
        rc = nvioctlNvhostAsGpu_InitializeEx(a->fd, 1, 0x100);

        if (R_FAILED(rc))
            nvasClose(a);
    }

    return rc;
}

Result nvasClose(NvAddressSpace* a)
{
    Result rc;

    rc = nvClose(a->fd);
    a->fd = -1;

    return rc;
}

Result nvasReserveAlign(NvAddressSpace* a, NvPageSize align, u32 pages, NvPageSize page_sz, iova_t* iova_out) {
    return nvioctlNvhostAsGpu_AllocSpace(a->fd, pages, page_sz, 0, align, iova_out);
}

Result nvasReserveAtFixedAddr(NvAddressSpace* a, iova_t addr, u32 pages, NvPageSize page_sz) {
    return nvioctlNvhostAsGpu_AllocSpace(a->fd, pages, page_sz, 1, addr, NULL);
}

Result nvasReserveFull(NvAddressSpace* a) {
    return nvasReserveAlign(a, NvPageSize_64K, 0x10000, NvPageSize_64K, NULL);
}

Result nvasMapBuffer(NvAddressSpace* a, NvBuffer* buffer, NvBufferKind kind, iova_t* iova_out) {
    return nvioctlNvhostAsGpu_MapBufferEx(a->fd, 0, kind, buffer->fd, 0, 0, buffer->size, 0, iova_out);
}
