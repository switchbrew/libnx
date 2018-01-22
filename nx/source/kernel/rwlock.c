// Copyright 2018 plutoo
#include "kernel/mutex.h"
#include "kernel/rwlock.h"

void rwlockReadLock(RwLock* r) {
    rmutexLock(&r->r);

    if (r->b++ == 0)
        rmutexLock(&r->g);

    rmutexUnlock(&r->r);
}

void rwlockReadUnlock(RwLock* r) {
    rmutexLock(&r->r);

    if (r->b-- == 1)
        rmutexUnlock(&r->g);

    rmutexUnlock(&r->r);
}

void rwlockWriteLock(RwLock* r) {
    rmutexLock(&r->g);
}

void rwlockWriteUnlock(RwLock* r) {
    rmutexUnlock(&r->g);
}
