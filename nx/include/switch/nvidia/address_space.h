#pragma once
#include "types.h"
#include "ioctl.h"

typedef struct NvAddressSpace {
    u32  fd;
    u32  page_size;
    bool has_init;
} NvAddressSpace;

Result nvAddressSpaceCreate(NvAddressSpace* a, u32 page_size);
void   nvAddressSpaceClose(NvAddressSpace* a);

Result nvAddressSpaceAlloc(NvAddressSpace* a, bool sparse, u64 size, iova_t* iova_out);
Result nvAddressSpaceAllocFixed(NvAddressSpace* a, bool sparse, u64 size, iova_t iova);
Result nvAddressSpaceFree(NvAddressSpace* a, iova_t iova, u64 size);

Result nvAddressSpaceMap(NvAddressSpace* a, u32 nvmap_handle, bool is_gpu_cacheable, NvKind kind, iova_t* iova_out);
Result nvAddressSpaceMapFixed(NvAddressSpace* a, u32 nvmap_handle, bool is_gpu_cacheable, NvKind kind, iova_t iova);
Result nvAddressSpaceModify(NvAddressSpace* a, iova_t iova, u64 offset, u64 size, NvKind kind);
Result nvAddressSpaceUnmap(NvAddressSpace* a, iova_t iova);
