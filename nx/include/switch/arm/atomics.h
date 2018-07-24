#pragma once
#include "../types.h"

static inline u32 atomicIncrement32(u32* p) {
    return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

static inline u32 atomicDecrement32(u32* p) {
    return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}

static inline u64 atomicIncrement64(u64* p) {
    return __atomic_fetch_add(p, 1, __ATOMIC_SEQ_CST);
}

static inline u64 atomicDecrement64(u64* p) {
    return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}
