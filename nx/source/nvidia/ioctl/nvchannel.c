#include <string.h>
#include "types.h"
#include "result.h"
#include "services/nv.h"
#include "nvidia/ioctl.h"
#include "runtime/hosversion.h"

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

Result nvioctlChannel_GetErrorInfo(u32 fd, NvError* out) {
    return nvIoctl(fd, _NV_IOR(0x48, 0x16, *out), out);
}

Result nvioctlChannel_GetErrorNotification(u32 fd, NvNotification* out) {
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

Result nvioctlChannel_Submit(u32 fd, const nvioctl_cmdbuf *cmdbufs, u32 num_cmdbufs, const nvioctl_reloc *relocs, const nvioctl_reloc_shift *reloc_shifts, u32 num_relocs,
        const nvioctl_syncpt_incr *syncpt_incrs, u32 num_syncpt_incrs, nvioctl_fence *fences, u32 num_fences) {
    // Make sure stack data doesn't get very large
    if (num_cmdbufs + num_relocs + num_syncpt_incrs + num_fences > 0x200)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    struct {
        __nv_in  u32                 num_cmdbufs;
        __nv_in  u32                 num_relocs;
        __nv_in  u32                 num_syncpt_incrs;
        __nv_in  u32                 num_fences;
        __nv_in  nvioctl_cmdbuf      cmdbufs     [num_cmdbufs];
        __nv_in  nvioctl_reloc       relocs      [num_relocs];
        __nv_in  nvioctl_reloc_shift reloc_shifts[num_relocs];
        __nv_in  nvioctl_syncpt_incr syncpt_incrs[num_syncpt_incrs];
        __nv_in  nvioctl_syncpt_incr wait_checks [num_syncpt_incrs];
        __nv_out nvioctl_fence       fences      [num_fences];
    } data;

    memset(&data, 0, sizeof(data));
    data.num_cmdbufs      = num_cmdbufs;
    data.num_relocs       = num_relocs;
    data.num_syncpt_incrs = num_syncpt_incrs;
    data.num_fences       = num_fences;
    memcpy(data.cmdbufs,      cmdbufs,      sizeof(data.cmdbufs));
    memcpy(data.relocs,       relocs,       sizeof(data.relocs));
    memcpy(data.reloc_shifts, reloc_shifts, sizeof(data.reloc_shifts));
    memcpy(data.syncpt_incrs, syncpt_incrs, sizeof(data.syncpt_incrs));

    Result rc = nvIoctl(fd, _NV_IOWR(0, 0x01, data), &data);

    if (R_SUCCEEDED(rc)) {
        memcpy(fences, data.fences, num_fences * sizeof(nvioctl_fence));
        for (int i = 0; i < num_fences; ++i)
            fences[i].id = data.syncpt_incrs[i].syncpt_id;
    }

    return rc;
}

Result nvioctlChannel_GetSyncpt(u32 fd, u32 id, u32 *syncpt) {
    struct {
        __nv_in  u32 id;
        __nv_out u32 syncpt;
    } data = {
        .id = id,
    };

    Result rc = nvIoctl(fd, _NV_IOWR(0, 0x02, data), &data);

    if (R_SUCCEEDED(rc) && syncpt)
        *syncpt = data.syncpt;

    return rc;
}

Result nvioctlChannel_GetModuleClockRate(u32 fd, u32 module_id, u32 *freq) {
    struct {
        __nv_out u32 rate;
        __nv_in  u32 module;
    } data = {
        .module = module_id,
    };

    u32 ioctl = _NV_IOWR(0, hosversionAtLeast(8,0,0) ? 0x14 : 0x23, data);
    Result rc = nvIoctl(fd, ioctl, &data);

    if (R_SUCCEEDED(rc) && freq)
        *freq = data.rate;

    return rc;
}

Result nvioctlChannel_MapCommandBuffer(u32 fd, nvioctl_command_buffer_map *maps, u32 num_maps, bool compressed) {
    if (num_maps > 0x200)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    struct {
        __nv_in    u32                        num_maps;
        __nv_in    u32                        reserved;
        __nv_in    u8                         is_compressed;
        __nv_inout nvioctl_command_buffer_map maps[num_maps];
    } data;

    memset(&data, 0, sizeof(data));
    data.num_maps      = num_maps;
    data.is_compressed = compressed;
    memcpy(data.maps, maps, sizeof(data.maps));

    Result rc = nvIoctl(fd, _NV_IOWR(0, 0x09, data), &data);

    if (R_SUCCEEDED(rc))
        memcpy(maps, data.maps, num_maps * sizeof(nvioctl_command_buffer_map));

    return rc;
}

Result nvioctlChannel_UnmapCommandBuffer(u32 fd, const nvioctl_command_buffer_map *maps, u32 num_maps, bool compressed) {
    if (num_maps > 0x200)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    struct {
        __nv_in    u32                        num_maps;
        __nv_in    u32                        reserved;
        __nv_in    u8                         is_compressed;
        __nv_inout nvioctl_command_buffer_map maps[num_maps];
    } data;

    memset(&data, 0, sizeof(data));
    data.num_maps      = num_maps;
    data.is_compressed = compressed;
    memcpy(data.maps, maps, sizeof(data.maps));

    return nvIoctl(fd, _NV_IOWR(0, 0x0a, data), &data);
}
