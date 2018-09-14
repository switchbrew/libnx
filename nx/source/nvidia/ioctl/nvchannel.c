#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"

Result nvioctlChannel_SetNvmapFd(u32 fd, u32 nvmap_fd) {
    struct {
        __nv_in u32 fd;
    } data;

    memset(&data, 0, sizeof(data));
    data.fd = nvmap_fd;

    return nvIoctl(fd, _NV_IOW(0x48, 0x01, data), &data);
}

Result nvioctlChannel_SubmitGpfifo(u32 fd, nvioctl_gpfifo_entry *entries, u32 num_entries, u32 flags, nvioctl_fence *fence_inout) {
    Result rc=0;

    // Make sure stack data doesn't get very large.
    if(num_entries > 0x200)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    struct {
        __nv_in    u64 gpfifo;           // (ignored) pointer to gpfifo entry structs
        __nv_in    u32 num_entries;      // number of entries being submitted
        __nv_in    u32 flags;
        __nv_inout nvioctl_fence fence;  // returned new fence object for others to wait on
        __nv_in    nvioctl_gpfifo_entry entries[num_entries]; // depends on num_entries
    } data;

    memset(&data, 0, sizeof(data));
    data.gpfifo = (u64)data.entries; // ignored
    data.num_entries = num_entries;
    data.flags = flags;
    data.fence = *fence_inout;
    memcpy(data.entries, entries, sizeof(data.entries));

    rc = nvIoctl(fd, _NV_IOWR(0x48, 0x08, data), &data);

    if (R_SUCCEEDED(rc)) {
        *fence_inout = data.fence;
    }

    return rc;
}

Result nvioctlChannel_KickoffPb(u32 fd, nvioctl_gpfifo_entry *entries, u32 num_entries, u32 flags, nvioctl_fence *fence_inout) {
    Result rc=0;

    struct {
        __nv_in    u64 gpfifo;           // (ignored) pointer to gpfifo entry structs
        __nv_in    u32 num_entries;      // number of entries being submitted
        __nv_in    u32 flags;
        __nv_inout nvioctl_fence fence;  // returned new fence object for others to wait on
    } data;

    memset(&data, 0, sizeof(data));
    data.gpfifo = (u64)entries; // ignored?
    data.num_entries = num_entries;
    data.flags = flags;
    data.fence = *fence_inout;

    rc = nvIoctl2(fd, _NV_IOWR(0x48, 0x1B, data), &data, entries, num_entries*sizeof(nvioctl_gpfifo_entry));

    if (R_SUCCEEDED(rc)) {
        *fence_inout = data.fence;
    }

    return rc;
}

Result nvioctlChannel_AllocObjCtx(u32 fd, u32 class_num, u32 flags, u64* id_out) {
    struct {
        __nv_in u32 class_num;
        __nv_in u32 flags;
        __nv_out u64 obj_id;  // (ignored) used for FREE_OBJ_CTX ioctl, which is not supported
    } data;

    memset(&data, 0, sizeof(data));
    data.class_num = class_num;
    data.flags = flags;
    data.obj_id = 0xDEADBEEF;

    Result rc = nvIoctl(fd, _NV_IOWR(0x48, 0x09, data), &data);

    if (R_SUCCEEDED(rc)) {
        if (id_out != NULL) {
            *id_out = data.obj_id;
        }
    }

    return rc;

}

Result nvioctlChannel_ZCullBind(u32 fd, u64 gpu_va, u32 mode) {
    struct {
        __nv_in u64 gpu_va;
        __nv_in u32 mode;
        __nv_in u32 padding;
    } data;

    memset(&data, 0, sizeof(data));
    data.gpu_va = gpu_va;
    data.mode = mode;

    return nvIoctl(fd, _NV_IOWR(0x48, 0x0B, data), &data);
}

Result nvioctlChannel_SetErrorNotifier(u32 fd, u32 enable) {
    struct {
        __nv_in  u64 offset;  // not used
        __nv_in  u64 size;    // not used
        __nv_in  u32 enable;
        u32 padding;
    } data;

    memset(&data, 0, sizeof(data));
    data.offset = 0;
    data.size = 0;
    data.enable = enable;

    return nvIoctl(fd, _NV_IOWR(0x48, 0x0C, data), &data);
}

Result nvioctlChannel_GetErrorNotification(u32 fd, NvError* out) {
    return nvIoctl(fd, _NV_IOWR(0x48, 0x17, *out), out);
}

Result nvioctlChannel_SetPriority(u32 fd, u32 priority) {
    struct {
        __nv_in u32 priority; // 0x32 is low, 0x64 is medium and 0x96 is high
    } data;

    memset(&data, 0, sizeof(data));
    data.priority = priority;

    return nvIoctl(fd, _NV_IOW(0x48, 0x0D, data), &data);
}

Result nvioctlChannel_SetTimeout(u32 fd, u32 timeout) {
    struct {
        __nv_in u32 timeout;
    } data;

    memset(&data, 0, sizeof(data));
    data.timeout = timeout;

    return nvIoctl(fd, _NV_IOW(0x48, 0x03, data), &data);
}

Result nvioctlChannel_AllocGpfifoEx2(u32 fd, u32 num_entries, u32 flags, u32 unk0, u32 unk1, u32 unk2, u32 unk3, nvioctl_fence *fence_out) {
    Result rc=0;

    struct {
        __nv_in  u32 num_entries;
        __nv_in  u32 flags;
        __nv_in  u32 unk0; // (1 works)
        __nv_out nvioctl_fence fence_out;
        __nv_in  u32 unk1;
        __nv_in  u32 unk2;
        __nv_in  u32 unk3;
    } data;

    memset(&data, 0, sizeof(data));
    data.num_entries = num_entries;
    data.flags = flags;
    data.unk0 = unk0;
    data.unk1 = unk1;
    data.unk2 = unk2;
    data.unk3 = unk3;

    rc = nvIoctl(fd, _NV_IOWR(0x48, 0x1A, data), &data);

    if (R_SUCCEEDED(rc) && fence_out) {
        memcpy(fence_out, &data.fence_out, sizeof(data.fence_out));
    }

    return rc;
}

Result nvioctlChannel_SetUserData(u32 fd, void* addr) {
    struct {
        __nv_in u64 addr;
    } data;

    memset(&data, 0, sizeof(data));
    data.addr = (u64)addr;

    return nvIoctl(fd, _NV_IOW(0x47, 0x14, data), &data);
}
