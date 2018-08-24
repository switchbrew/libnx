// Copyright 2018 plutoo
#include "kernel/mutex.h"
#include "kernel/rwlock.h"

void rwlockInit(RwLock* r) {
    rmutexInit(&r->r);
    rmutexInit(&r->g);
    r->b = 0;
}

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
