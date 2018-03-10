#pragma once

typedef struct {
    u32  fd;
    bool has_init;
} NvAddressSpace;

typedef enum {
    NvPageSize_4K  = 0x1000,
    NvPageSize_64K = 0x10000
} NvPageSize;

typedef u64 iova_t;


Result nvasCreate(NvAddressSpace* a);
void   nvasClose(NvAddressSpace* a);

Result nvasReserveAlign(NvAddressSpace* a, NvPageSize align, u32 pages, NvPageSize page_sz, iova_t* iova_out);
Result nvasReserveAtFixedAddr(NvAddressSpace* a, iova_t addr, u32 pages, NvPageSize page_sz);
Result nvasReserveFull(NvAddressSpace* a);

Result nvasMapBuffer(NvAddressSpace* a, NvBuffer* buffer, iova_t* iova_out);

struct NvChannel;
Result nvasBindToChannel(NvAddressSpace* a, struct NvChannel* channel);
