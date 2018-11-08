#pragma once
#include "types.h"

typedef struct NvMap {
    u32    handle;
    u32    id;
    u32    size;
    void*  cpu_addr;
    NvKind kind;
    bool   has_init;
    bool   is_cpu_cacheable;
} NvMap;

Result nvMapInit(void);
u32    nvMapGetFd(void);
void   nvMapExit(void);

Result nvMapCreate(NvMap* m, void* cpu_addr, u32 size, u32 align, NvKind kind, bool is_cpu_cacheable);
void   nvMapFree(NvMap* m);

static inline u32 nvMapGetHandle(NvMap* m)
{
    return m->handle;
}

static inline u32 nvMapGetId(NvMap* m)
{
    return m->id;
}

static inline void* nvMapGetCpuAddr(NvMap* m)
{
    return m->cpu_addr;
}
