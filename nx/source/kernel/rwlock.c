// Copyright 2018 plutoo
#include "kernel/mutex.h"
#include "kernel/rwlock.h"

void rwlockInit(RwLock* r) {
    mutexInit(&r->mutex);
    condvarInit(&r->condvar_readers);
    condvarInit(&r->condvar_writer);

    r->readers = 0;
    r->writer = false;
}

void rwlockReadLock(RwLock* r) {
    mutexLock(&r->mutex);

    while (r->writer) {
        condvarWait(&r->condvar_writer, &r->mutex);
    }

    r->readers++;

    mutexUnlock(&r->mutex);
}

void rwlockReadUnlock(RwLock* r) {
    mutexLock(&r->mutex);

    if (--r->readers == 0) {
        condvarWakeAll(&r->condvar_readers);
    }

    mutexUnlock(&r->mutex);
}

void rwlockWriteLock(RwLock* r) {
    mutexLock(&r->mutex);

    while (r->writer) {
        condvarWait(&r->condvar_writer, &r->mutex);
    }

    r->writer = true;

    while (r->readers > 0) {
        condvarWait(&r->condvar_readers, &r->mutex);
    }

    mutexUnlock(&r->mutex);
}

void rwlockWriteUnlock(RwLock* r) {
    mutexLock(&r->mutex);

    r->writer = false;
    condvarWakeAll(&r->condvar_writer);

    mutexUnlock(&r->mutex);
}
