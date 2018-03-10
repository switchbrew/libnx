#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu*   parent;
    NvBuffer ctx_buf;
} NvZcullContext;

Result nvzcullCreate(NvZcullContext* z, NvGpu* parent);
void   nvzcullClose(NvZcullContext* z);
