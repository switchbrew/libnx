#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

Result nvioctlNvhostCtrlGpu_ZCullGetCtxSize(u32 fd, u32 *out) {
    Result rc = 0;

    struct {
        __nv_out u32 out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x01, data), &data);

    if (R_SUCCEEDED(rc)) {
        *out = data.out;
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, nvioctl_zcull_info *out) {
    Result rc = 0;

    struct {
        __nv_out nvioctl_zcull_info out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x02, data), &data);

    if (R_SUCCEEDED(rc)) {
        *out = data.out;
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZbcSetTable(u32 fd, const u32 color_ds[4], const u32 color_l2[4], u32 depth, u32 format, u32 type) {
    Result rc = 0;

    struct {
        __nv_in u32 color_ds[4];
        __nv_in u32 color_l2[4];
        __nv_in u32 depth;
        __nv_in u32 format;
        __nv_in u32 type;         // 1=color, 2=depth
    } data;

    memset(&data, 0, sizeof(data));
    if (color_ds) memcpy(data.color_ds, color_ds, sizeof(data.color_ds));
    if (color_l2) memcpy(data.color_l2, color_l2, sizeof(data.color_l2));
    data.depth = depth;
    data.format = format;
    data.type = type;

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x03, data), &data);

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZbcQueryTable(u32 fd, u32 index, nvioctl_zbc_entry *out) {
    Result rc = 0;

    struct {
        __nv_inout nvioctl_zbc_entry entry;
    } data;

    memset(&data, 0, sizeof(data));
    data.entry.size = index;

    rc = nvIoctl(fd, _NV_IOWR(0x47, 0x04, data), &data);

    if (R_SUCCEEDED(rc)) {
        *out = data.entry;
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetCharacteristics(u32 fd, nvioctl_gpu_characteristics *out) {
    Result rc = 0;

    struct {
        __nv_in  u64 gc_buf_size;   // must not be NULL, but gets overwritten with 0xA0=max_size
        __nv_in  u64 gc_buf_addr;   // ignored, but must not be NULL
        __nv_out nvioctl_gpu_characteristics gc;
    } data;

    memset(&data, 0, sizeof(data));
    data.gc_buf_size = sizeof(nvioctl_gpu_characteristics);
    data.gc_buf_addr = 1;

    rc = nvIoctl(fd, _NV_IOWR(0x47, 0x05, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, &data.gc, sizeof(nvioctl_gpu_characteristics));
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, void *buffer, size_t size) {
    Result rc = 0;

    struct {
        __nv_in u32 bufsize;
        __nv_in u32 _padding;
        __nv_in u64 bufaddr;
        __nv_out u8 out[8];
    } data;

    memset(&data, 0, sizeof(data));
    data.bufsize = size;
    data.bufaddr = (u64)buffer;

    rc = nvIoctl(fd, _NV_IOWR(0x47, 0x06, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(buffer, data.out, size);
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZbcGetActiveSlotMask(u32 fd, nvioctl_zbc_slot_mask *out) {
    Result rc = 0;

    struct {
        __nv_out nvioctl_zbc_slot_mask out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x14, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, &data.out, sizeof(data.out));
    }

    return rc;
}
