#pragma once
#include "types.h"
#include "address_space.h"

typedef enum {
    NvBufferFlags_Writable=1,
} NvBufferFlags;

typedef struct NvAddressSpace NvAddressSpace;

typedef struct NvBuffer {
    u32    fd;
    u32    size;
    void*  cpu_addr;
    iova_t gpu_addr;
    iova_t gpu_addr_texture;
    NvAddressSpace* addr_space;
    NvKind kind;
    bool   has_init;
} NvBuffer;

Result nvBufferInit(void);
u32    nvBufferGetNvmapFd(void);
void   nvBufferExit(void);

Result nvBufferCreate(NvBuffer* m, size_t size, u32 align, NvKind kind, NvAddressSpace* as);
Result nvBufferCreateRw(NvBuffer* m, size_t size, u32 align, NvKind kind, NvAddressSpace* as);
void   nvBufferFree(NvBuffer* m);

void*  nvBufferGetCpuAddr(NvBuffer* m);
iova_t nvBufferGetGpuAddr(NvBuffer* m);

Result nvBufferMapAsTexture(NvBuffer* m, NvKind kind);
iova_t nvBufferGetGpuAddrTexture(NvBuffer* m);

Result nvBufferMakeCpuUncached(NvBuffer* m);
Result nvBufferMakeCpuCached(NvBuffer* m);

