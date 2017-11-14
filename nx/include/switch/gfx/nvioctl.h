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
 } gpu_characteristics;

typedef struct {
    u64 offset;
    u32 page_size;
    u32 pad;
    u64 pages;
} nvioctl_va_region;

Result nvioctlNvhostCtrlGpu_ZCullGetCtxSize(u32 fd, u32 *out);
Result nvioctlNvhostCtrlGpu_ZCullGetInfo(u32 fd, u32 out[40>>2]);
Result nvioctlNvhostCtrlGpu_GetCharacteristics(u32 fd, gpu_characteristics *out);
Result nvioctlNvhostCtrlGpu_GetTpcMasks(u32 fd, u32 inval, u32 out[24>>2]);

Result nvioctlNvhostAsGpu_AllocSpace(u32 fd, u32 pages, u32 page_size, u32 flags, u64 align, u64 *offset);
Result nvioctlNvhostAsGpu_MapBufferEx(u32 fd, u32 flags, u32 kind, u32 nvmap_handle, u32 page_size, u64 buffer_offset, u64 mapping_size, u64 *offset);
Result nvioctlNvhostAsGpu_GetVARegions(u32 fd, nvioctl_va_region regions[2]);
Result nvioctlNvhostAsGpu_InitializeEx(u32 fd, u32 big_page_size);

Result nvioctlNvmap_Create(u32 fd, u32 size, u32 *nvmap_handle);
Result nvioctlNvmap_Alloc(u32 fd, u32 nvmap_handle, u32 heapmask, u32 flags, u32 align, u8 kind, void* addr);

