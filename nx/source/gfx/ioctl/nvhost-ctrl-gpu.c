#include <string.h>
#include <switch/types.h>
#include <switch/result.h>
#include <switch/services/nv.h>
#include <switch/gfx/ioctl.h>
#include <switch/gfx/nvioctl.h>

Result nvioctlNvhostCtrlGpu_ZCullGetCtxSize(u32 fd, u32 *out) {
    Result rc = 0;

    struct {
        __out u32 out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x01, data), &data);

    if (R_SUCCEEDED(rc)) {
        *out = data.out;
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, u32 out[40>>2]) {
    Result rc = 0;

    struct {
        __out u32 out[40>>2];
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x02, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, data.out, sizeof(data.out));
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetCharacteristics(u32 fd, gpu_characteristics *out) {
    Result rc = 0;

    struct {
        __in  u64 gc_buf_size;   // must not be NULL, but gets overwritten with 0xA0=max_size
        __in  u64 gc_buf_addr;   // ignored, but must not be NULL
        __out gpu_characteristics gc;
    } data;

    memset(&data, 0, sizeof(data));
    data.gc_buf_size = sizeof(gpu_characteristics);
    data.gc_buf_addr = 1;

    rc = nvIoctl(fd, _IOWR(0x47, 0x05, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, &data.gc, sizeof(gpu_characteristics));
    }

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, u32 inval, u32 out[24>>2]) {
    Result rc = 0;

    // Fixme: This one is wrong.
    struct {
        __inout u32 unk[24>>2];
    } data;

    memset(&data, 0, sizeof(data));
    data.unk[0] = inval;
    data.unk[2] = 1; //addr?

    rc = nvIoctl(fd, _IOWR(0x47, 0x06, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, &data.unk, sizeof(data.unk));

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetL2State(u32 fd, nvioctl_l2_state *out) {
    Result rc = 0;

    struct {
        __out nvioctl_l2_state out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x14, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(out, &data.out, sizeof(data.out));
    }

    return rc;
}
