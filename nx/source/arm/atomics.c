#include "types.h"

u32 atomicIncrement32(u32* p) {
    return __sync_fetch_and_add(p, 1);
}

u32 atomicDecrement32(u32* p) {
    return __sync_sub_and_fetch(p, 1);
}

u64 atomicIncrement64(u64* p) {
    return __sync_fetch_and_add(p, 1);
}

u64 atomicDecrement64(u64* p) {
    return __sync_sub_and_fetch(p, 1);
}
