#include <string.h>
#include <switch.h>

Result nvioctlNvhostCtrlGpu_GetCharacteristics(u32 fd, gpu_characteristics *out) {
    Result rc = 0;

    struct {
        u64 gpu_characteristics_buf_size;   // in/out (must not be NULL, but gets overwritten with 0xA0=max_size)
        u64 gpu_characteristics_buf_addr;   // in (ignored, but must not be NULL)
        gpu_characteristics gc;      // out
    } data;

    memset(&data, 0, sizeof(data));
    data.gpu_characteristics_buf_size = sizeof(gpu_characteristics);
    data.gpu_characteristics_buf_addr = 1;

    rc = nvIoctl(fd, _IOWR(0x47, 0x05, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, &data.gc, sizeof(gpu_characteristics));

    return rc;
}

Result nvioctlNvhostAsGpu_AllocSpace(u32 fd, u32 pages, u32 page_size, u32 flags, u64 align, u64 *offset) {
    Result rc=0;

    struct {
        u32 pages;//in
        u32 page_size;//in
        u32 flags;//in
        u32      pad;
        union {
        u64 offset;//out
        u64 align;//in
        };
    } data;

    memset(&data, 0, sizeof(data));
    data.pages = pages;
    data.page_size = page_size;
    data.flags = flags;
    data.align = align;

    rc = nvIoctl(fd, _IOWR(0x41, 0x02, data), &data);
    if (R_FAILED(rc)) return rc;

    *offset = data.offset;

    return rc;
}

Result nvioctlNvhostAsGpu_MapBufferEx(u32 fd, u32 flags, u32 kind, u32 nvmap_handle, u32 page_size, u64 buffer_offset, u64 mapping_size, u64 *offset) {
    Result rc=0;

    struct {
        u32 flags;          // in bit0: fixed_offset, bit2: cacheable
        u32 kind;           // in -1 is default
        u32 nvmap_handle;   // in
        u32 page_size;      // inout 0 means don't care
        u64 buffer_offset;  // in
        u64 mapping_size;   // in
        u64 offset;         // out
    } data;

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    data.kind = kind;
    data.nvmap_handle = nvmap_handle;
    data.page_size = page_size;
    data.buffer_offset = buffer_offset;
    data.mapping_size = mapping_size;

    rc = nvIoctl(fd, _IOW(0x41, 0x06, data), &data);
    if (R_FAILED(rc)) return rc;

    if (offset) *offset = data.offset;

    return rc;
}

Result nvioctlNvhostAsGpu_InitializeEx(u32 fd, u32 big_page_size) {
    struct {
        u32 big_page_size;   // depends on GPU's available_big_page_sizes; 0=default
        s32 as_fd;           // ignored; passes 0
        u32 flags;           // ignored; passes 0
        u32 reserved;        // ignored; passes 0
        u64 unk0;
        u64 unk1;
        u64 unk2;
    } data;

    memset(&data, 0, sizeof(data));
    data.big_page_size = big_page_size;

    return nvIoctl(fd, _IOW(0x41, 0x09, data), &data);

}

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        u32 size;//in
        u32 handle;//out
    } data;

    memset(&data, 0, sizeof(data));
    data.size = size;

    rc = nvIoctl(fd, _IOWR(0x01, 0x01, data), &data);
    if (R_FAILED(rc)) return rc;

    *nvmap_handle = data.handle;

    return rc;
}

Result nvioctlNvmap_Alloc(u32 fd, u32 nvmap_handle, u32 heapmask, u32 flags, u32 align, u8 kind, void* addr) {
    struct {
        u32 handle;//in
        u32 heapmask;//in
        u32 flags;    //in (0=read-only, 1=read-write)
        u32 align;//in
        u8  kind;//in
        u8       pad[7];
        u64 addr;//in
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;
    data.heapmask = heapmask;
    data.flags = flags;
    data.align = align;
    data.kind = kind;
    data.addr = (u64)addr;

    return nvIoctl(fd, _IOWR(0x01, 0x04, data), &data);
}

