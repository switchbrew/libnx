#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu* parent;
} Nv3dContext;

Result nv3dCreate(Nv3dContext* t, NvGpu* parent);
void   nv3dClose(Nv3dContext* t);
