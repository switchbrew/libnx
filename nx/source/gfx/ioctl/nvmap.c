#include <string.h>
#include <switch/types.h>
#include <switch/result.h>
#include <switch/services/nv.h>
#include <switch/gfx/ioctl.h>
#include <switch/gfx/nvioctl.h>

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        __in  u32 size;
        __out u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.size = size;

    rc = nvIoctl(fd, _IOWR(0x01, 0x01, data), &data);

    if (R_SUCCEEDED(rc)) {
        *nvmap_handle = data.handle;
    }

    return rc;
}

Result nvioctlNvmap_FromId(u32 fd, u32 id, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        __in  u32 id;
        __out u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.id = id;

    rc = nvIoctl(fd, _IOWR(0x01, 0x03, data), &data);

    if (R_SUCCEEDED(rc)) {
        *nvmap_handle = data.handle;
    }

    return rc;
}

Result nvioctlNvmap_Alloc(u32 fd, u32 nvmap_handle, u32 heapmask, u32 flags, u32 align, u8 kind, void* addr) {
    struct {
        __in u32 handle;
        __in u32 heapmask;
        __in u32 flags;    // (0=read-only, 1=read-write)
        __in u32 align;
        __in u8  kind;
        u8       pad[7];
        __in u64 addr;
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

Result nvioctlNvmap_GetId(u32 fd, u32 nvmap_handle, u32 *id) {
    Result rc=0;

    struct {
        __out u32 id;
        __in  u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;

    rc = nvIoctl(fd, _IOWR(0x01, 0x0E, data), &data);

    if (R_SUCCEEDED(rc)) {
        *id = data.id;
    }

    return rc;
}
