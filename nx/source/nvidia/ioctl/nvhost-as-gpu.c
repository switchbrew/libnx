#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

Result nvioctlNvhostAsGpu_BindChannel(u32 fd, u32 channel_fd) {
    struct {
        __nv_in u32 fd;
    } data;

    memset(&data, 0, sizeof(data));
    data.fd = channel_fd;

    return nvIoctl(fd, _NV_IOW(0x41, 0x01, data), &data);
}

Result nvioctlNvhostAsGpu_AllocSpace(u32 fd, u32 pages, u32 page_size, u32 flags, u64 align_or_offset, u64 *offset) {
    Result rc = 0;

    struct {
        __nv_in u32 pages;
        __nv_in u32 page_size;
        __nv_in u32 flags; // bit0=fixed bit1=sparse
        u32      pad;
        union {
            __nv_out u64 offset;
            __nv_in  u64 align;
        };
    } data;

    memset(&data, 0, sizeof(data));
    data.pages = pages;
    data.page_size = page_size;
    data.flags = flags;
    data.align = align_or_offset;

    rc = nvIoctl(fd, _NV_IOWR(0x41, 0x02, data), &data);
    if (R_FAILED(rc)) return rc;

    if (offset != NULL)
        *offset = data.offset;

    return rc;
}

Result nvioctlNvhostAsGpu_FreeSpace(u32 fd, u64 offset, u32 pages, u32 page_size) {
    struct {
        __nv_in u64 offset;
        __nv_in u32 pages;
        __nv_in u32 page_size;
    } data;

    memset(&data, 0, sizeof(data));
    data.offset = offset;
    data.pages = pages;
    data.page_size = page_size;

    return nvIoctl(fd, _NV_IOWR(0x41, 0x03, data), &data);
}

Result nvioctlNvhostAsGpu_MapBufferEx(
    u32 fd, u32 flags, u32 kind, u32 nvmap_handle, u32 page_size,
    u64 buffer_offset, u64 mapping_size, u64 input_offset, u64 *offset)
{
    Result rc = 0;

    struct {
        __nv_in u32 flags;        // bit0: fixed_offset, bit2: cacheable
        __nv_in u32 kind;         // -1 is default
        __nv_in u32 nvmap_handle;
        __nv_inout u32 page_size; // 0 means don't care
        __nv_in u64 buffer_offset;
        __nv_in u64 mapping_size;
        __nv_inout u64 offset;
    } data;

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    data.kind = kind;
    data.nvmap_handle = nvmap_handle;
    data.page_size = page_size;
    data.buffer_offset = buffer_offset;
    data.mapping_size = mapping_size;
    data.offset = input_offset;

    rc = nvIoctl(fd, _NV_IOWR(0x41, 0x06, data), &data);

    if (R_SUCCEEDED(rc) && offset) {
        *offset = data.offset;
    }

    return rc;
}

Result nvioctlNvhostAsGpu_UnmapBuffer(u32 fd, u64 offset) {
    struct {
        __nv_in u64 offset;
    } data;

    memset(&data, 0, sizeof(data));
    data.offset = offset;

    return nvIoctl(fd, _NV_IOWR(0x41, 0x05, data), &data);
}

Result nvioctlNvhostAsGpu_GetVARegions(u32 fd, nvioctl_va_region regions[2]) {
    Result rc=0;

    struct {
        u64 not_used;        // contained output user ptr on linux, ignored
        __nv_inout u32 bufsize; // forced to 2*sizeof(struct va_region)
        u32 pad;
        __nv_out nvioctl_va_region regions[2];
    } data;

    memset(&data, 0, sizeof(data));
    data.bufsize = sizeof(data.regions);

    rc = nvIoctl(fd, _NV_IOWR(0x41, 0x08, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(regions, data.regions, sizeof(data.regions));
    }

    return rc;
}

Result nvioctlNvhostAsGpu_InitializeEx(u32 fd, u32 flags, u32 big_page_size) {
    struct {
        __nv_in u32 flags;           // flags? passes 1
        __nv_in s32 as_fd;           // ignored; passes 0
        __nv_in u32 big_page_size;   // depends on GPU's available_big_page_sizes; 0=default
        __nv_in u32 reserved;        // ignored; passes 0
        __nv_in u64 unk0;
        __nv_in u64 unk1;
        __nv_in u64 unk2;
    } data;

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    data.big_page_size = big_page_size;

    return nvIoctl(fd, _NV_IOW(0x41, 0x09, data), &data);
}
