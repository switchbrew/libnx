#pragma once

typedef struct NvGpu NvGpu;

typedef struct {
    NvGpu* parent;
} NvZcullContext;

Result nvzcullCreate(NvZcullContext* z, NvGpu* parent);
void   nvzcullClose(NvZcullContext* z);
