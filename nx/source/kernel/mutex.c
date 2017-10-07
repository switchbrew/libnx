// Copyright 2017 plutoo
#include <switch.h>

#define HAS_LISTENERS 0x40000000

static u32 _GetTag() {
    // todo: Needs filling in at thread creation.
    // todo: Must always be assigned non-zero.
    return ((u32*)armGetTls()) [0x1FC/4];
}

void mutexLock(Mutex* m) {
    u32 self = _GetTag();
    u32 cur = __sync_val_compare_and_swap(&m->Tag, 0, self);

    while (1) {
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
            svcArbitrateLock(cur &~ HAS_LISTENERS, &m->Tag, self);
        }
        else {
            // The flag is not set, we need to set it.
            u32 old = __sync_val_compare_and_swap(&m->Tag, cur, cur | HAS_LISTENERS);

            if (old == cur) {
                // Flag was set successfully.
                svcArbitrateLock(cur &~ HAS_LISTENERS, &m->Tag, self);
            }
        }

        cur = __sync_val_compare_and_swap(&m->Tag, 0, self);
    }
}

void mutexUnlock(Mutex* m) {
    u32 self = _GetTag();
    u32 old = __sync_val_compare_and_swap(&m->Tag, self, 0);

    if (old & HAS_LISTENERS) {
        svcArbitrateUnlock(&m->Tag);
    }
}

void rmutexLock(RMutex* m) {
    if (m->Owner == _GetTag()) {
        m->Count++;
    }
    else {
        mutexLock(&m->Lock);
        m->Owner = _GetTag();
    }
}

void rmutexUnlock(RMutex* m) {
    if (--m->Count == 0) {
        m->Owner = 0;
        mutexUnlock(&m->Lock);
    }
}
