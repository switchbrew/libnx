// Copyright 2018 plutoo
#include "kernel/mutex.h"
#include "kernel/rwlock.h"
#include "../internal.h"

NX_INLINE u32 _GetCurrentThreadTag(void) {
    return getThreadVars()->handle;
}

void rwlockInit(RwLock* r) {
    mutexInit(&r->mutex);
    condvarInit(&r->condvar_reader_wait);
    condvarInit(&r->condvar_writer_wait);

    r->read_lock_count = 0;
    r->read_waiter_count = 0;
    r->write_lock_count = 0;
    r->write_waiter_count = 0;
    r->write_owner_tag = 0;
}

void rwlockReadLock(RwLock* r) {
    const u32 cur_tag = _GetCurrentThreadTag();

    if (r->write_owner_tag == cur_tag) {
        r->read_lock_count++;
        return;
    }

    mutexLock(&r->mutex);

    while (r->write_waiter_count > 0) {
        r->read_waiter_count++;
        condvarWait(&r->condvar_reader_wait, &r->mutex);
        r->read_waiter_count--;
    }

    r->read_lock_count++;

    mutexUnlock(&r->mutex);
}

bool rwlockTryReadLock(RwLock* r) {
    const u32 cur_tag = _GetCurrentThreadTag();

    if (r->write_owner_tag == cur_tag) {
        r->read_lock_count++;
        return true;
    }

    if (!mutexTryLock(&r->mutex)) {
        return false;
    }

    const bool got_lock = r->write_waiter_count == 0;
    if (got_lock) {
        r->read_lock_count++;
    }

    mutexUnlock(&r->mutex);
    return got_lock;
}

void rwlockReadUnlock(RwLock* r) {
    const u32 cur_tag = _GetCurrentThreadTag();

    if (r->write_owner_tag == cur_tag) {
        // Write lock is owned by this thread.
        r->read_lock_count--;
        if (r->read_lock_count == 0 && r->write_lock_count == 0) {
            // Relinquish control of the lock.
            r->write_owner_tag = 0;

            if (r->write_waiter_count > 0) {
                condvarWakeOne(&r->condvar_writer_wait);
            } else if (r->read_waiter_count > 0) {
                condvarWakeAll(&r->condvar_reader_wait);
            }

            // Corresponding mutexLock was called in WriteLock/WriteTryLock.
            mutexUnlock(&r->mutex);
        }
    } else {
        // Write lock isn't owned by this thread.
        mutexLock(&r->mutex);
        r->read_lock_count--;
        if (r->read_lock_count == 0 && r->write_waiter_count > 0) {
            condvarWakeOne(&r->condvar_writer_wait);
        }
        mutexUnlock(&r->mutex);
    }
}

void rwlockWriteLock(RwLock* r) {
    const u32 cur_tag = _GetCurrentThreadTag();

    if (r->write_owner_tag == cur_tag) {
        r->write_lock_count++;
        return;
    }

    mutexLock(&r->mutex);

    while (r->read_lock_count > 0) {
        r->write_waiter_count++;
        condvarWait(&r->condvar_writer_wait, &r->mutex);
        r->write_waiter_count--;
    }

    r->write_lock_count = 1;
    r->write_owner_tag = cur_tag;

    // mutexUnlock(&r->mutex) is intentionally not called here.
    // The mutex will be unlocked by a call to ReadUnlock or WriteUnlock.
}

bool rwlockTryWriteLock(RwLock* r) {
    const u32 cur_tag = _GetCurrentThreadTag();

    if (r->write_owner_tag == cur_tag) {
        r->write_lock_count++;
        return true;
    }

    if (!mutexTryLock(&r->mutex)) {
        return false;
    }

    if (r->read_lock_count > 0) {
        mutexUnlock(&r->mutex);
        return false;
    }

    r->write_lock_count = 1;
    r->write_owner_tag = cur_tag;

    // mutexUnlock(&r->mutex) is intentionally not called here.
    // The mutex will be unlocked by a call to ReadUnlock or WriteUnlock.
    return true;
}

void rwlockWriteUnlock(RwLock* r) {
    // This function assumes the write lock is held.
    // This means that r->mutex is locked, and r->write_owner_tag == cur_tag.
    // if (r->write_owner_tag == cur_tag)
    {
        r->write_lock_count--;
        if (r->write_lock_count == 0 && r->read_lock_count == 0) {
            // Relinquish control of the lock.
            r->write_owner_tag = 0;

            if (r->write_waiter_count > 0) {
                condvarWakeOne(&r->condvar_writer_wait);
            } else if (r->read_waiter_count > 0) {
                condvarWakeAll(&r->condvar_reader_wait);
            }

            // Corresponding mutexLock was called in WriteLock/WriteTryLock.
            mutexUnlock(&r->mutex);
        }
    }
}

bool rwlockIsWriteLockHeldByCurrentThread(RwLock* r) {
    return r->write_owner_tag == _GetCurrentThreadTag() && r->write_lock_count > 0;
}

bool rwlockIsOwnedByCurrentThread(RwLock* r) {
    return r->write_owner_tag == _GetCurrentThreadTag();
}
