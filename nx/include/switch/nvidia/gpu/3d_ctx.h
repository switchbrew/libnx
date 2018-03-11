#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu* parent;
    u64    obj_id;
} Nv3DContext;

Result nv3DContextCreate(Nv3DContext* t, NvGpu* parent);
void   nv3DContextClose(Nv3DContext* t);
