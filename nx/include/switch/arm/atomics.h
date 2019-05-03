/**
 * @file atomics.h
 * @brief AArch64 atomic operations.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Atomically increments a 32-bit value.
static inline u32 atomicIncrement32(u32* p) {
    return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

/// Atomically decrements a 32-bit value.
static inline u32 atomicDecrement32(u32* p) {
    return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}

/// Atomically increments a 64-bit value.
static inline u64 atomicIncrement64(u64* p) {
    return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

/// Atomically decrements a 64-bit value.
static inline u64 atomicDecrement64(u64* p) {
    return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}
