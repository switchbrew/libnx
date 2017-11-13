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

