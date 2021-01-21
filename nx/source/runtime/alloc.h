#pragma once
#include "types.h"

void* __libnx_alloc(size_t size);
void* __libnx_aligned_alloc(size_t alignment, size_t size);
void __libnx_free(void* p);
