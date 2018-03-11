#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu*   parent;
    NvBuffer ctx_buf;
} NvZcullContext;

Result nvZcullContextCreate(NvZcullContext* z, NvGpu* parent);
void   nvZcullContextClose(NvZcullContext* z);
