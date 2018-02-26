#pragma once

typedef struct {
    NvChannel* parent;
    nvioctl_fence fifo_fence;
} NvGpfifo;

Result nvfifoCreate(NvGpfifo* f, NvChannel* parent);
void   nvfifoClose();
