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

Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, u32 out[40>>2]) {
    Result rc = 0;

    struct {
        __nv_out u32 out[40>>2];
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x02, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, data.out, sizeof(data.out));
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

Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, u32 inval, u32 out[24>>2]) {
    Result rc = 0;

    // Fixme: This one is wrong.
    struct {
        __nv_inout u32 unk[24>>2];
    } data;

    memset(&data, 0, sizeof(data));
    data.unk[0] = inval;
    data.unk[2] = 1; //addr?

    rc = nvIoctl(fd, _NV_IOWR(0x47, 0x06, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, &data.unk, sizeof(data.unk));

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetL2State(u32 fd, nvioctl_l2_state *out) {
    Result rc = 0;

    struct {
        __nv_out nvioctl_l2_state out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _NV_IOR(0x47, 0x14, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, &data.out, sizeof(data.out));
    }

    return rc;
}
