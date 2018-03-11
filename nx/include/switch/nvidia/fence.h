#pragma once

typedef struct {
    nvioctl_fence fence;
} NvFence;

static inline void nvfenceCreate(NvFence* f, nvioctl_fence* fence) {
    f->fence = *fence;
}
