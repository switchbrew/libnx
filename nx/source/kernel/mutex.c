// Copyright 2017 plutoo, 2020 SciresM
#include "result.h"
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "../internal.h"

#define HANDLE_WAIT_MASK 0x40000000u

#define LIKELY(expr)   (__builtin_expect_with_probability(!!(expr), 1, 1.0))
#define UNLIKELY(expr) (__builtin_expect_with_probability(!!(expr), 0, 1.0))

NX_INLINE u32 _GetTag(void) {
    return getThreadVars()->handle;
}

NX_INLINE u32 _LoadExclusive(u32 *ptr) {
    u32 value;
    __asm__ __volatile__("ldaxr %w[value], %[ptr]" : [value]"=&r"(value) : [ptr]"Q"(*ptr) : "memory");
    return value;
}

NX_INLINE int _StoreExclusive(u32 *ptr, u32 value) {
    int result;
    __asm__ __volatile__("stlxr %w[result], %w[value], %[ptr]" : [result]"=&r"(result) : [value]"r"(value), [ptr]"Q"(*ptr) : "memory");
    return result;
}

NX_INLINE void _ClearExclusive(void) {
    __asm__ __volatile__("clrex" ::: "memory");
}

void mutexLock(Mutex* m) {
    // Get the current thread handle.
    const u32 cur_handle = _GetTag();

    u32 value = _LoadExclusive(m);
    while (true) {
        // If the mutex isn't owned, try to take it.
        if (LIKELY(value == INVALID_HANDLE)) {
            // If we fail, try again.
            if (UNLIKELY(_StoreExclusive(m, cur_handle) != 0)) {
                value = _LoadExclusive(m);
                continue;
            }
            break;
        }

        // If the mutex doesn't have any waiters, try to register ourselves as the first waiter.
        if (LIKELY((value & HANDLE_WAIT_MASK) == 0)) {
            // If we fail, try again.
            if (UNLIKELY(_StoreExclusive(m, value | HANDLE_WAIT_MASK) != 0)) {
                value = _LoadExclusive(m);
                continue;
            }
        }

        // Ask the kernel to arbitrate the lock for us.
        if (UNLIKELY(R_FAILED(svcArbitrateLock(value & ~HANDLE_WAIT_MASK, m, cur_handle)))) {
            // This should be impossible under normal circumstances.
            svcBreak(BreakReason_Assert, 0, 0);
        }

        // Reload the value, and check if we got the lock.
        value = _LoadExclusive(m);
        if (LIKELY((value & ~HANDLE_WAIT_MASK) == cur_handle)) {
            _ClearExclusive();
            break;
        }
    }

    // __dmb(); // Done only in aarch32 mode.
}

bool mutexTryLock(Mutex* m) {
    // Get the current thread handle.
    const u32 cur_handle = _GetTag();

    while (true) {
        // Check that the mutex is not owned.
        u32 value = _LoadExclusive(m);
        if (UNLIKELY(value != INVALID_HANDLE)) {
            break;
        }

        // __dmb(); // Done only in aarch32 mode.

        if (LIKELY(_StoreExclusive(m, cur_handle) == 0)) {
            return true;
        }
    }

    // Release our exclusive hold.
    _ClearExclusive();

    // __dmb(); // Done only in aarch32 mode.

    return false;
}

void mutexUnlock(Mutex* m) {
    // Get the current thread handle.
    const u32 cur_handle = _GetTag();

    u32 value = _LoadExclusive(m);
    while (true) {
        // If we have any listeners, we need to ask the kernel to arbitrate.
        if (UNLIKELY(value != cur_handle)) {
            _ClearExclusive();
            break;
        }

        // __dmb(); // Done only in aarch32 mode.

        // Try to release the lock.
        if (LIKELY(_StoreExclusive(m, INVALID_HANDLE) == 0)) {
            break;
        }

        // Reload the value and try again.
        value = _LoadExclusive(m);
    }

    // __dmb(); // Done only in aarch32 mode.

    if (value & HANDLE_WAIT_MASK) {
        // Ask the kernel to arbitrate unlock for us.
        if (UNLIKELY(R_FAILED(svcArbitrateUnlock(m)))) {
            // This should be impossible under normal circumstances.
            svcBreak(BreakReason_Assert, 0, 0);
        }
    }
}

bool mutexIsLockedByCurrentThread(const Mutex* m) {
    // Get the current thread handle.
    const u32 cur_handle = _GetTag();

    return (*m & ~HANDLE_WAIT_MASK) == cur_handle;
}


void rmutexLock(RMutex* m) {
    if (!mutexIsLockedByCurrentThread(&m->lock)) {
        mutexLock(&m->lock);
    }
    m->counter++;
}

bool rmutexTryLock(RMutex* m) {
    if (!mutexIsLockedByCurrentThread(&m->lock)) {
        if (!mutexTryLock(&m->lock)) {
            return false;
        }
    }
    m->counter++;
    return true;
}

void rmutexUnlock(RMutex* m) {
    if (--m->counter == 0) {
        mutexUnlock(&m->lock);
    }
}
