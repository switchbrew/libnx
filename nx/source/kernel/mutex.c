// Copyright 2017 plutoo
#include "kernel/svc.h"
#include "kernel/mutex.h"
#include "../internal.h"

#define HAS_LISTENERS 0x40000000

static u32 _GetTag(void) {
    return getThreadVars()->handle;
}

void mutexLock(Mutex* m) {
    u32 self = _GetTag();

    while (1) {
        u32 cur = __sync_val_compare_and_swap((u32*)m, 0, self);

        if (cur == 0) {
            // We won the race!
            return;
        }

        if ((cur &~ HAS_LISTENERS) == self) {
            // Kernel assigned it to us!
            return;
        }

        if (cur & HAS_LISTENERS) {
            // The flag is already set, we can use the syscall.
            svcArbitrateLock(cur &~ HAS_LISTENERS, (u32*)m, self);
        }
        else {
            // The flag is not set, we need to set it.
            u32 old = __sync_val_compare_and_swap((u32*)m, cur, cur | HAS_LISTENERS);

            if (old == cur) {
                // Flag was set successfully.
                svcArbitrateLock(cur, (u32*)m, self);
            }
        }
    }
}

bool mutexTryLock(Mutex* m) {
    u32 self = _GetTag();
    u32 cur = __sync_val_compare_and_swap((u32*)m, 0, self);

    if (cur == 0) {
        // We won the race!
        return true;
    }

    if ((cur &~ HAS_LISTENERS) == self) {
        // Kernel assigned it to us!
        return true;
    }
    
    return 0;
}

void mutexUnlock(Mutex* m) {
    u32 old = __sync_val_compare_and_swap((u32*)m, _GetTag(), 0);

    if (old & HAS_LISTENERS) {
        svcArbitrateUnlock((u32*)m);
    }
}

void rmutexLock(RMutex* m) {
    if (m->thread_tag != _GetTag()) {
        mutexLock(&m->lock);
        m->thread_tag = _GetTag();
    }

    m->counter++;
}

bool rmutexTryLock(RMutex* m) {
    if (m->thread_tag != _GetTag()) {
        if (!mutexTryLock(&m->lock)) {
            return false;
        }
        m->thread_tag = _GetTag();
    }

    m->counter++;
    return true;
}

void rmutexUnlock(RMutex* m) {
    if (--m->counter == 0) {
        m->thread_tag = 0;
        mutexUnlock(&m->lock);
    }
}
