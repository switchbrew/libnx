#pragma once
#include "ioctl.h"

typedef nvioctl_fence NvFence;

typedef struct {
    u32 num_fences;
    NvFence fences[4];
} NvMultiFence;

Result nvFenceWait(NvFence* f, s32 timeout_ms);

static inline void nvMultiFenceCreate(NvMultiFence* mf, const NvFence* fence) {
    mf->num_fences = 1;
    mf->fences[0] = *fence;
}

Result nvMultiFenceWait(NvMultiFence* mf, s32 timeout_ms);
