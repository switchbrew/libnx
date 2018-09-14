#pragma once
#include "types.h"

typedef struct NvAddressSpace {
    u32  fd;
    bool has_init;
} NvAddressSpace;

typedef enum {
    NvPageSize_4K  = 0x1000,
    NvPageSize_64K = 0x10000
} NvPageSize;

Result nvAddressSpaceCreate(NvAddressSpace* a);
void   nvAddressSpaceClose(NvAddressSpace* a);

Result nvAddressSpaceReserveAlign(NvAddressSpace* a, NvPageSize align, u32 pages, NvPageSize page_sz, iova_t* iova_out);
Result nvAddressSpaceReserveAtFixedAddr(NvAddressSpace* a, iova_t addr, u32 pages, NvPageSize page_sz);
Result nvAddressSpaceReserveFull(NvAddressSpace* a);

Result nvAddressSpaceMapBuffer(NvAddressSpace* a, u32 fd, u32 flags, NvKind kind, iova_t* iova_out);
Result nvAddressSpaceUnmapBuffer(NvAddressSpace* a, iova_t iova);

struct NvChannel;
Result nvAddressSpaceBindToChannel(NvAddressSpace* a, struct NvChannel* channel);
