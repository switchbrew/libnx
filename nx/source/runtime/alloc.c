#include "alloc.h"
#include <stdlib.h>

void* __attribute__((weak)) __libnx_alloc(size_t size) {
    return malloc(size);
}

void* __attribute__((weak)) __libnx_aligned_alloc(size_t alignment, size_t size) {
    size = (size + alignment - 1) &~ (alignment - 1);
    return aligned_alloc(alignment, size);
}

void __attribute__((weak)) __libnx_free(void* p) {
    free(p);
}
