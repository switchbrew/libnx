#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        __nv_in  u32 size;
        __nv_out u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.size = size;

    rc = nvIoctl(fd, _NV_IOWR(0x01, 0x01, data), &data);

    if (R_SUCCEEDED(rc)) {
        *nvmap_handle = data.handle;
    }

    return rc;
}

Result nvioctlNvmap_FromId(u32 fd, u32 id, u32 *nvmap_handle) {
    Result rc=0;

    struct {
        __nv_in  u32 id;
        __nv_out u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.id = id;

    rc = nvIoctl(fd, _NV_IOWR(0x01, 0x03, data), &data);

    if (R_SUCCEEDED(rc)) {
        *nvmap_handle = data.handle;
    }

    return rc;
}

Result nvioctlNvmap_Alloc(u32 fd, u32 nvmap_handle, u32 heapmask, u32 flags, u32 align, u8 kind, void* addr) {
    struct {
        __nv_in u32 handle;
        __nv_in u32 heapmask;
        __nv_in u32 flags;    // (0=read-only, 1=read-write)
        __nv_in u32 align;
        __nv_in u8  kind;
        u8       pad[7];
        __nv_in u64 addr;
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;
    data.heapmask = heapmask;
    data.flags = flags;
    data.align = align;
    data.kind = kind;
    data.addr = (u64)addr;

    return nvIoctl(fd, _NV_IOWR(0x01, 0x04, data), &data);
}

Result nvioctlNvmap_Free(u32 fd, u32 nvmap_handle) {
    struct {
        __nv_in u32 handle;
        u32 pad;
        __nv_out u64 refcount;
        __nv_out u32 size;
        __nv_out u32 flags; // 1=NOT_FREED_YET
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;

    return nvIoctl(fd, _NV_IOWR(0x01, 0x05, data), &data);
}

Result nvioctlNvmap_Param(u32 fd, u32 nvmap_handle, NvMapParam param, u32 *result) {
    Result rc=0;

    struct {
        __nv_in  u32 handle;
        __nv_in  u32 param;
        __nv_out u32 result;
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;
    data.param = param;

    rc = nvIoctl(fd, _NV_IOWR(0x01, 0x09, data), &data);

    if (R_SUCCEEDED(rc)) {
        *result = data.result;
    }

    return rc;
}

Result nvioctlNvmap_GetId(u32 fd, u32 nvmap_handle, u32 *id) {
    Result rc=0;

    struct {
        __nv_out u32 id;
        __nv_in  u32 handle;
    } data;

    memset(&data, 0, sizeof(data));
    data.handle = nvmap_handle;

    rc = nvIoctl(fd, _NV_IOWR(0x01, 0x0E, data), &data);

    if (R_SUCCEEDED(rc)) {
        *id = data.id;
    }

    return rc;
}
