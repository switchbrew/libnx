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
Result nvMapLoadRemote(NvMap* m, u32 id);
void   nvMapClose(NvMap* m);

static inline u32 nvMapGetHandle(NvMap* m)
{
    return m->handle;
}

static inline u32 nvMapGetId(NvMap* m)
{
    return m->id;
}

static inline u32 nvMapGetSize(NvMap* m)
{
    return m->size;
}

static inline void* nvMapGetCpuAddr(NvMap* m)
{
    return m->cpu_addr;
}

static inline bool nvMapIsRemote(NvMap* m)
{
    return !m->cpu_addr;
}

static inline NvKind nvMapGetKind(NvMap* m)
{
    return m->kind;
}
