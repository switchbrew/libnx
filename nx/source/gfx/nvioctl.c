#include <string.h>
#include <switch.h>

Result nvioctlNvhostCtrl_EventSignal(u32 fd, u32 event_id) {
    struct {
        u32 event_id;      //in ranges from 0x01 to 0x3F
    } data;

    memset(&data, 0, sizeof(data));
    data.event_id = event_id;

    return nvIoctl(fd, _IOWR(0x00, 0x1C, data), &data);
}

Result nvioctlNvhostCtrl_EventWait(u32 fd, u32 unk0, u32 unk1, s32 timeout, u32 event_id, u32 *out) {
    Result rc = 0;

    struct {
        u32 unk0;//in
        u32 unk1;//in
        s32 timeout;//in
        u32 event;// in=event_id; out=result
    } data;

    memset(&data, 0, sizeof(data));
    data.unk0 = unk0;
    data.unk1 = unk1;
    data.timeout = timeout;
    data.event = event_id;

    rc = nvIoctl(fd, _IOWR(0x00, 0x1D, data), &data);
    if (R_FAILED(rc)) return rc;

    if(out) *out = data.event;

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZCullGetCtxSize(u32 fd, u32 *out) {
    Result rc = 0;

    struct {
        u32 out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x01, data), &data);
    if (R_FAILED(rc)) return rc;

    *out = data.out;

    return rc;
}

Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, u32 out[40>>2]) {
    Result rc = 0;

    struct {
        u32 out[40>>2];
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x02, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, data.out, sizeof(data.out));

    return rc;
}

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

Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, u32 inval, u32 out[24>>2]) {
    Result rc = 0;

    struct {
        u32 unk[24>>2];
    } data;

    memset(&data, 0, sizeof(data));
    data.unk[0] = inval;
    data.unk[2] = 1;//addr?

    rc = nvIoctl(fd, _IOWR(0x47, 0x06, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, &data.unk, sizeof(data.unk));

    return rc;
}

Result nvioctlNvhostCtrlGpu_GetL2State(u32 fd, nvioctl_l2_state *out) {
    Result rc = 0;

    struct {
        nvioctl_l2_state out;
    } data;

    memset(&data, 0, sizeof(data));

    rc = nvIoctl(fd, _IOR(0x47, 0x14, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(out, &data.out, sizeof(data.out));

    return rc;
}

Result nvioctlNvhostAsGpu_BindChannel(u32 fd, u32 channel_fd) {
    struct {
        u32 fd;//in
    } data;

    memset(&data, 0, sizeof(data));
    data.fd = channel_fd;

    return nvIoctl(fd, _IOW(0x41, 0x01, data), &data);
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

Result nvioctlNvhostAsGpu_MapBufferEx(u32 fd, u32 flags, u32 kind, u32 nvmap_handle, u32 page_size, u64 buffer_offset, u64 mapping_size, u64 input_offset, u64 *offset) {
    Result rc=0;

    struct {
        u32 flags;          // in bit0: fixed_offset, bit2: cacheable
        u32 kind;           // in -1 is default
        u32 nvmap_handle;   // in
        u32 page_size;      // inout 0 means don't care
        u64 buffer_offset;  // in
        u64 mapping_size;   // in
        u64 offset;         // inout
    } data;

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    data.kind = kind;
    data.nvmap_handle = nvmap_handle;
    data.page_size = page_size;
    data.buffer_offset = buffer_offset;
    data.mapping_size = mapping_size;
    data.offset = input_offset;

    rc = nvIoctl(fd, _IOWR(0x41, 0x06, data), &data);
    if (R_FAILED(rc)) return rc;

    if (offset) *offset = data.offset;

    return rc;
}

Result nvioctlNvhostAsGpu_GetVARegions(u32 fd, nvioctl_va_region regions[2]) {
    Result rc=0;
 
    struct {
        u64 not_used;   // (contained output user ptr on linux, ignored)
        u32 bufsize;    //inout forced to 2*sizeof(struct va_region)
        u32 pad;
        nvioctl_va_region regions[2];//out
    } data;

    memset(&data, 0, sizeof(data));
    data.bufsize = sizeof(data.regions);

    rc = nvIoctl(fd, _IOWR(0x41, 0x08, data), &data);
    if (R_FAILED(rc)) return rc;

    memcpy(regions, data.regions, sizeof(data.regions));

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

Result nvioctlChannel_SetNvmapFd(u32 fd, u32 nvmap_fd) {
    struct {
        u32 fd;//in
    } data;

    memset(&data, 0, sizeof(data));
    data.fd = nvmap_fd;

    return nvIoctl(fd, _IOW(0x48, 0x01, data), &data);
}

Result nvioctlChannel_SubmitGPFIFO(u32 fd, nvioctl_gpfifo_entry *entries, u32 num_entries, u32 flags, nvioctl_fence *fence_out) {
    Result rc=0;

    if(num_entries > 0x200) return MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);//Make sure stack data doesn't get very large.

    struct {
        u64 gpfifo;                      // in (ignored) pointer to gpfifo fence structs
        u32 num_entries;                 // in number of fence objects being submitted
        u32 flags;                       // in
        nvioctl_fence fence_out;          // out returned new fence object for others to wait on
        nvioctl_gpfifo_entry entries[num_entries];   // in depends on num_entries
    } data;


    memset(&data, 0, sizeof(data));
    data.gpfifo = 1;
    data.num_entries = num_entries;
    data.flags = flags;
    memcpy(data.entries, entries, sizeof(data.entries));

    rc = nvIoctl(fd, _IOWR(0x48, 0x08, data), &data);
    if (R_FAILED(rc)) return rc;

    if(fence_out) memcpy(fence_out, &data.fence_out, sizeof(data.fence_out));

    return rc;
}

Result nvioctlChannel_AllocObjCtx(u32 fd, u32 class_num, u32 flags) {
    struct {
        u32 class_num;    // 0x902D=2d, 0xB197=3d, 0xB1C0=compute, 0xA140=kepler, 0xB0B5=DMA, 0xB06F=channel_gpfifo
        u32 flags;
        u64 obj_id;       // (ignored) used for FREE_OBJ_CTX ioctl, which is not supported
    } data;

    memset(&data, 0, sizeof(data));
    data.class_num = class_num;
    data.flags = flags;
    data.obj_id = 0xDEADBEEF;

    return nvIoctl(fd, _IOWR(0x48, 0x09, data), &data);
}

Result nvioctlChannel_ZCullBind(u32 fd, u32 in[4]) {
    struct {
        u32 in[4];
    } data;

    memset(&data, 0, sizeof(data));
    memcpy(data.in, in, sizeof(data.in));

    return nvIoctl(fd, _IOWR(0x48, 0x0B, data), &data);
}

Result nvioctlChannel_SetErrorNotifier(u32 fd, u64 offset, u64 size, u32 nvmap_handle) {
    struct {
        u64 offset;//in
        u64 size;//in
        u32 mem;       //in nvmap object handle
        u32 padding;//in
    } data;

    memset(&data, 0, sizeof(data));
    data.offset = offset;
    data.size = size;
    data.mem = nvmap_handle;

    return nvIoctl(fd, _IOWR(0x48, 0x0C, data), &data);
}

Result nvioctlChannel_SetPriority(u32 fd, u32 priority) {
    struct {
        u32 priority;    //in 0x32 is low, 0x64 is medium and 0x96 is high
    } data;

    memset(&data, 0, sizeof(data));
    data.priority = priority;

    return nvIoctl(fd, _IOW(0x48, 0x0D, data), &data);
}

Result nvioctlChannel_AllocGPFIFOEx2(u32 fd, u32 num_entries, u32 flags, u32 unk0, u32 unk1, u32 unk2, u32 unk3, nvioctl_fence *fence_out) {
    Result rc=0;

    struct {
        u32 num_entries;         // in
        u32 flags;               // in
        u32 unk0;                // in (1 works)
        nvioctl_fence fence_out;  // out
        u32 unk1;                // in
        u32 unk2;                // in
        u32 unk3;                // in
    } data;

    memset(&data, 0, sizeof(data));
    data.num_entries = num_entries;
    data.flags = flags;
    data.unk0 = unk0;
    data.unk1 = unk1;
    data.unk2 = unk2;
    data.unk3 = unk3;

    rc = nvIoctl(fd, _IOWR(0x48, 0x1A, data), &data);
    if (R_FAILED(rc)) return rc;

    if(fence_out) memcpy(fence_out, &data.fence_out, sizeof(data.fence_out));

    return rc;
}

Result nvioctlChannel_SetUserData(u32 fd, void* addr) {
    struct {
        u64 addr;
    } data;

    memset(&data, 0, sizeof(data));
    data.addr = (u64)addr;

    return nvIoctl(fd, _IOW(0x47, 0x14, data), &data);
}

