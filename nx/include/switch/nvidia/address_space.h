#pragma once

typedef struct {
    u32 fd;
} NvAddressSpace;

typedef enum {
    NvPageSize_4K  = 0x1000,
    NvPageSize_64K = 0x10000
} NvPageSize;

typedef u64 iova_t;


Result nvasCreate(NvAddressSpace* a);
Result nvasClose(NvAddressSpace* a);

Result nvasReserveAlign(NvAddressSpace* a, NvPageSize align, u32 pages, NvPageSize page_sz, iova_t* iova_out);
Result nvasReserveAtFixedAddr(NvAddressSpace* a, iova_t addr, u32 pages, NvPageSize page_sz);
Result nvasReserveFull(NvAddressSpace* a);

Result nvasMapBuffer(NvAddressSpace* a, Nvmap* buffer, NvmapKind kind, iova_t* iova_out);
