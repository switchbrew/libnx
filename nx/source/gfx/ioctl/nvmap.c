#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "gfx/ioctl.h"
#include "gfx/nvioctl.h"

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        _in_  u32 size;
        _out_ u32 handle;
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
        _in_  u32 id;
        _out_ u32 handle;
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
        _in_ u32 handle;
        _in_ u32 heapmask;
        _in_ u32 flags;    // (0=read-only, 1=read-write)
        _in_ u32 align;
        _in_ u8  kind;
        u8       pad[7];
        _in_ u64 addr;
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
        _out_ u32 id;
        _in_  u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;

    rc = nvIoctl(fd, _IOWR(0x01, 0x0E, data), &data);

    if (R_SUCCEEDED(rc)) {
        *id = data.id;
    }

    return rc;
}
