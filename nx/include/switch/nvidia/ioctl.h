#pragma once
#include "types.h"

// The below defines are based on Linux kernel ioctl.h.
#define _NV_IOC_NRBITS	8
#define _NV_IOC_TYPEBITS	8
#define _NV_IOC_SIZEBITS	14
#define _NV_IOC_DIRBITS	2

#define _NV_IOC_NRMASK	((1 << _NV_IOC_NRBITS)-1)
#define _NV_IOC_TYPEMASK	((1 << _NV_IOC_TYPEBITS)-1)
#define _NV_IOC_SIZEMASK	((1 << _NV_IOC_SIZEBITS)-1)
#define _NV_IOC_DIRMASK	((1 << _NV_IOC_DIRBITS)-1)

#define _NV_IOC_NRSHIFT	0
#define _NV_IOC_TYPESHIFT	(_NV_IOC_NRSHIFT+_NV_IOC_NRBITS)
#define _NV_IOC_SIZESHIFT	(_NV_IOC_TYPESHIFT+_NV_IOC_TYPEBITS)
#define _NV_IOC_DIRSHIFT	(_NV_IOC_SIZESHIFT+_NV_IOC_SIZEBITS)

// Direction bits.
#define _NV_IOC_NONE	0U
#define _NV_IOC_WRITE	1U
#define _NV_IOC_READ	2U

#define _NV_IOC(dir,type,nr,size) \
	(((dir)  << _NV_IOC_DIRSHIFT) | \
	 ((type) << _NV_IOC_TYPESHIFT) | \
	 ((nr)   << _NV_IOC_NRSHIFT) | \
	 ((size) << _NV_IOC_SIZESHIFT))

/* used to create numbers */
#define _NV_IO(type,nr)		_NV_IOC(_NV_IOC_NONE,(type),(nr),0)
#define _NV_IOR(type,nr,size)	_NV_IOC(_NV_IOC_READ,(type),(nr),sizeof(size))
#define _NV_IOW(type,nr,size)	_NV_IOC(_NV_IOC_WRITE,(type),(nr),sizeof(size))
#define _NV_IOWR(type,nr,size)	_NV_IOC(_NV_IOC_READ|_NV_IOC_WRITE,(type),(nr),sizeof(size))

/* used to decode ioctl numbers.. */
#define _NV_IOC_DIR(nr)		(((nr) >> _NV_IOC_DIRSHIFT) & _NV_IOC_DIRMASK)
#define _NV_IOC_TYPE(nr)		(((nr) >> _NV_IOC_TYPESHIFT) & _NV_IOC_TYPEMASK)
#define _NV_IOC_NR(nr)		(((nr) >> _NV_IOC_NRSHIFT) & _NV_IOC_NRMASK)
#define _NV_IOC_SIZE(nr)		(((nr) >> _NV_IOC_SIZESHIFT) & _NV_IOC_SIZEMASK)

#define __nv_in
#define __nv_out
#define __nv_inout

typedef struct {
    u32 width_align_pixels;            // 0x20  (32)
    u32 height_align_pixels;           // 0x20  (32)
    u32 pixel_squares_by_aliquots;     // 0x400 (1024)
    u32 aliquot_total;                 // 0x800 (2048)
    u32 region_byte_multiplier;        // 0x20  (32)
    u32 region_header_size;            // 0x20  (32)
    u32 subregion_header_size;         // 0xC0  (192)
    u32 subregion_width_align_pixels;  // 0x20  (32)
    u32 subregion_height_align_pixels; // 0x40  (64)
    u32 subregion_count;               // 0x10  (16)
} nvioctl_zcull_info;

typedef struct {
    u32 color_ds[4];
    u32 color_l2[4];
    u32 depth;
    u32 ref_cnt;
    u32 format;
    u32 type;
    u32 size;
} nvioctl_zbc_entry;

typedef struct {
    u32 arch;                           // 0x120 (NVGPU_GPU_ARCH_GM200)
    u32 impl;                           // 0xB (NVGPU_GPU_IMPL_GM20B)
    u32 rev;                            // 0xA1 (Revision A1)
    u32 num_gpc;                        // 0x1
    u64 L2_cache_size;                  // 0x40000
    u64 on_board_video_memory_size;     // 0x0 (not used)
    u32 num_tpc_per_gpc;                // 0x2
    u32 bus_type;                       // 0x20 (NVGPU_GPU_BUS_TYPE_AXI)
    u32 big_page_size;                  // 0x20000
    u32 compression_page_size;          // 0x20000
    u32 pde_coverage_bit_count;         // 0x1B
    u32 available_big_page_sizes;       // 0x30000
    u32 gpc_mask;                       // 0x1
    u32 sm_arch_sm_version;             // 0x503 (Maxwell Generation 5.0.3?)
    u32 sm_arch_spa_version;            // 0x503 (Maxwell Generation 5.0.3?)
    u32 sm_arch_warp_count;             // 0x80
    u32 gpu_va_bit_count;               // 0x28
    u32 reserved;                       // NULL
    u64 flags;                          // 0x55
    u32 twod_class;                     // 0x902D (FERMI_TWOD_A)
    u32 threed_class;                   // 0xB197 (MAXWELL_B)
    u32 compute_class;                  // 0xB1C0 (MAXWELL_COMPUTE_B)
    u32 gpfifo_class;                   // 0xB06F (MAXWELL_CHANNEL_GPFIFO_A)
    u32 inline_to_memory_class;         // 0xA140 (KEPLER_INLINE_TO_MEMORY_B)
    u32 dma_copy_class;                 // 0xB0B5 (MAXWELL_DMA_COPY_A)
    u32 max_fbps_count;                 // 0x1
    u32 fbp_en_mask;                    // 0x0 (disabled)
    u32 max_ltc_per_fbp;                // 0x2
    u32 max_lts_per_ltc;                // 0x1
    u32 max_tex_per_tpc;                // 0x0 (not supported)
    u32 max_gpc_count;                  // 0x1
    u32 rop_l2_en_mask_0;               // 0x21D70 (fuse_status_opt_rop_l2_fbp_r)
    u32 rop_l2_en_mask_1;               // 0x0
    u64 chipname;                       // 0x6230326D67 ("gm20b")
    u64 gr_compbit_store_base_hw;       // 0x0 (not supported)
} nvioctl_gpu_characteristics;

typedef struct {
    u64 offset;
    u32 page_size;
    u32 pad;
    u64 pages;
} nvioctl_va_region;

typedef struct {
    u32 slot;        // always 0x07 (?)
    u32 mask;
} nvioctl_zbc_slot_mask;

typedef struct {
    u64 timestamp;
    u64 reserved;
} nvioctl_gpu_time;

typedef struct {
    u32 id;
    u32 value;
} nvioctl_fence;

typedef struct {
    union {
        u64 desc;
        u32 desc32[2];
    };
} nvioctl_gpfifo_entry;

typedef struct {
    u32 mem;
    u32 offset;
    u32 words;
} nvioctl_cmdbuf;

typedef struct {
    u32 cmdbuf_mem;
    u32 cmdbuf_offset;
    u32 target;
    u32 target_offset;
} nvioctl_reloc;

typedef struct {
    u32 shift;
} nvioctl_reloc_shift;

typedef struct {
    u32 syncpt_id;
    u32 syncpt_incrs;
} nvioctl_syncpt_incr;

typedef struct {
    u32 handle;
    u32 iova;
} nvioctl_command_buffer_map;

#define NVGPU_ZBC_TYPE_INVALID     0
#define NVGPU_ZBC_TYPE_COLOR       1
#define NVGPU_ZBC_TYPE_DEPTH       2

// Used with nvioctlNvmap_Param().
typedef enum nvioctl_map_param {
    NvMapParam_Size = 1,
    NvMapParam_Alignment = 2,
    NvMapParam_Base = 3,
    NvMapParam_Heap = 4,
    NvMapParam_Kind = 5
} NvMapParam;

// Used with nvioctlChannel_AllocObjCtx().
typedef enum nvioctl_channel_obj_classnum {
    NvClassNumber_2D = 0x902D,
    NvClassNumber_3D = 0xB197,
    NvClassNumber_Compute = 0xB1C0,
    NvClassNumber_Kepler = 0xA140,
    NvClassNumber_DMA = 0xB0B5,
    NvClassNumber_ChannelGpfifo = 0xB06F
} NvClassNumber;

// Used with nvioctlChannel_SetPriority().
typedef enum nvioctl_channel_priority {
    NvChannelPriority_Low    = 50,
    NvChannelPriority_Medium = 100,
    NvChannelPriority_High   = 150
} NvChannelPriority;

// Used with nvioctlChannel_ZCullBind().
typedef enum {
    NvZcullConfig_Global = 0,
    NvZcullConfig_NoCtxSwitch = 1,
    NvZcullConfig_SeparateBuffer = 2,
    NvZcullConfig_PartOfRegularBuffer = 3
} NvZcullConfig;

// Used with nvioctlNvhostAsGpu_AllocSpace().
typedef enum {
    NvAllocSpaceFlags_FixedOffset = 1,
    NvAllocSpaceFlags_Sparse = 2,
} NvAllocSpaceFlags;

// Used with nvioctlNvhostAsGpu_MapBufferEx().
typedef enum {
    NvMapBufferFlags_FixedOffset = 1,
    NvMapBufferFlags_IsCacheable = 4,
    NvMapBufferFlags_Modify = 0x100,
} NvMapBufferFlags;

typedef enum {
    NvNotificationType_FifoErrorIdleTimeout=8,
    NvNotificationType_GrErrorSwNotify=13,
    NvNotificationType_GrSemaphoreTimeout=24,
    NvNotificationType_GrIllegalNotify=25,
    NvNotificationType_FifoErrorMmuErrFlt=31,
    NvNotificationType_PbdmaError=32,
    NvNotificationType_ResetChannelVerifError=43,
    NvNotificationType_PbdmaPushbufferCrcMismatch=80
} NvNotificationType;

typedef struct {
    u64 timestamp;
    u32 info32; // see NvNotificationType
    u16 info16;
    u16 status; // always -1
} NvNotification;

typedef struct {
    u32 type;
    u32 info[31];
} NvError;

Result nvioctlNvhostCtrl_SyncptRead(u32 fd, u32 id, u32* out);
Result nvioctlNvhostCtrl_SyncptIncr(u32 fd, u32 id);
Result nvioctlNvhostCtrl_SyncptWait(u32 fd, u32 id, u32 threshold, u32 timeout);
Result nvioctlNvhostCtrl_EventSignal(u32 fd, u32 event_id);
Result nvioctlNvhostCtrl_EventWait(u32 fd, u32 syncpt_id, u32 threshold, s32 timeout, u32 event_id, u32 *out);
Result nvioctlNvhostCtrl_EventWaitAsync(u32 fd, u32 syncpt_id, u32 threshold, s32 timeout, u32 event_id);
Result nvioctlNvhostCtrl_EventRegister(u32 fd, u32 event_id);
Result nvioctlNvhostCtrl_EventUnregister(u32 fd, u32 event_id);

Result nvioctlNvhostCtrlGpu_ZCullGetCtxSize(u32 fd, u32 *out);
Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, nvioctl_zcull_info *out);
Result nvioctlNvhostCtrlGpu_ZbcSetTable(u32 fd, const u32 color_ds[4], const u32 color_l2[4], u32 depth, u32 format, u32 type);
Result nvioctlNvhostCtrlGpu_ZbcQueryTable(u32 fd, u32 index, nvioctl_zbc_entry *out);
Result nvioctlNvhostCtrlGpu_GetCharacteristics(u32 fd, nvioctl_gpu_characteristics *out);
Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, void *buffer, size_t size);
Result nvioctlNvhostCtrlGpu_ZbcGetActiveSlotMask(u32 fd, nvioctl_zbc_slot_mask *out);
Result nvioctlNvhostCtrlGpu_GetGpuTime(u32 fd, nvioctl_gpu_time *out);

Result nvioctlNvhostAsGpu_BindChannel(u32 fd, u32 channel_fd);
Result nvioctlNvhostAsGpu_AllocSpace(u32 fd, u32 pages, u32 page_size, u32 flags, u64 align_or_offset, u64 *offset);
Result nvioctlNvhostAsGpu_FreeSpace(u32 fd, u64 offset, u32 pages, u32 page_size);
Result nvioctlNvhostAsGpu_MapBufferEx(u32 fd, u32 flags, u32 kind, u32 nvmap_handle, u32 page_size, u64 buffer_offset, u64 mapping_size, u64 input_offset, u64 *offset);
Result nvioctlNvhostAsGpu_UnmapBuffer(u32 fd, u64 offset);
Result nvioctlNvhostAsGpu_GetVARegions(u32 fd, nvioctl_va_region regions[2]);
Result nvioctlNvhostAsGpu_InitializeEx(u32 fd, u32 flags, u32 big_page_size);

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle);
Result nvioctlNvmap_FromId(u32 fd, u32 id, u32 *nvmap_handle);
Result nvioctlNvmap_Alloc(u32 fd, u32 nvmap_handle, u32 heapmask, u32 flags, u32 align, u8 kind, void* addr);
Result nvioctlNvmap_Free(u32 fd, u32 nvmap_handle);
Result nvioctlNvmap_Param(u32 fd, u32 nvmap_handle, NvMapParam param, u32 *result);
Result nvioctlNvmap_GetId(u32 fd, u32 nvmap_handle, u32 *id);

Result nvioctlChannel_SetNvmapFd(u32 fd, u32 nvmap_fd);
Result nvioctlChannel_SubmitGpfifo(u32 fd, nvioctl_gpfifo_entry *entries, u32 num_entries, u32 flags, nvioctl_fence *fence_inout);
Result nvioctlChannel_KickoffPb(u32 fd, nvioctl_gpfifo_entry *entries, u32 num_entries, u32 flags, nvioctl_fence *fence_inout);
Result nvioctlChannel_AllocObjCtx(u32 fd, u32 class_num, u32 flags, u64* id_out);
Result nvioctlChannel_ZCullBind(u32 fd, u64 gpu_va, u32 mode);
Result nvioctlChannel_SetErrorNotifier(u32 fd, u32 enable);
Result nvioctlChannel_GetErrorInfo(u32 fd, NvError* out);
Result nvioctlChannel_GetErrorNotification(u32 fd, NvNotification* out);
Result nvioctlChannel_SetPriority(u32 fd, u32 priority);
Result nvioctlChannel_SetTimeout(u32 fd, u32 timeout);
Result nvioctlChannel_AllocGpfifoEx2(u32 fd, u32 num_entries, u32 flags, u32 unk0, u32 unk1, u32 unk2, u32 unk3, nvioctl_fence *fence_out);
Result nvioctlChannel_SetUserData(u32 fd, void* addr);
Result nvioctlChannel_Submit(u32 fd, const nvioctl_cmdbuf *cmdbufs, u32 num_cmdbufs, const nvioctl_reloc *relocs, const nvioctl_reloc_shift *reloc_shifts, u32 num_relocs,
    const nvioctl_syncpt_incr *syncpt_incrs, u32 num_syncpt_incrs, nvioctl_fence *fences, u32 num_fences);
Result nvioctlChannel_GetSyncpt(u32 fd, u32 module_id, u32 *syncpt);
Result nvioctlChannel_GetModuleClockRate(u32 fd, u32 module_id, u32 *freq);
Result nvioctlChannel_MapCommandBuffer(u32 fd, nvioctl_command_buffer_map *maps, u32 num_maps, bool compressed);
Result nvioctlChannel_UnmapCommandBuffer(u32 fd, const nvioctl_command_buffer_map *maps, u32 num_maps, bool compressed);
