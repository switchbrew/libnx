#pragma once

typedef struct {
    NvBuffer buffer;
} NvCmdList;

static inline iova_t nvcmdsGetGpuAddr() {
    return 0; // TODO
}

static inline u64 nvcmdsGetListSize() {
    return 0; // TODO
}

