// Copyright 2018 plutoo
#pragma once
#include "../types.h"

typedef enum {
    JitType_CodeMemory,
    JitType_JitMemory
} JitType;

typedef struct {
    JitType type;
    size_t  size;
    void*   src_addr;
    void*   rx_addr;
    void*   rw_addr;
    Handle  handle;
} Jit;

Result jitCreate(Jit* j, size_t size);
Result jitTransitionToWritable(Jit* j);
Result jitTransitionToExecutable(Jit* j);
Result jitClose(Jit* j);

void* jitGetRwAddr(Jit* j);
void* jitGetRxAddr(Jit* j);
