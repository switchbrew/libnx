/**
 * @file rwlock.h
 * @brief Read/write lock synchronization primitive.
 * @author plutoo, SciresM
 * @copyright libnx Authors
 */
#pragma once
#include "../kernel/mutex.h"
#include "../kernel/condvar.h"

/// Read/write lock structure.
typedef struct {
    Mutex mutex;
    CondVar condvar_reader_wait;
    CondVar condvar_writer_wait;
    u32 read_lock_count;
    u32 read_waiter_count;
    u32 write_lock_count;
    u32 write_waiter_count;
    u32 write_owner_tag;
} RwLock;

/**
 * @brief Initializes the read/write lock.
 * @param r Read/write lock object.
 */
void rwlockInit(RwLock* r);

/**
 * @brief Locks the read/write lock for reading.
 * @param r Read/write lock object.
 */
void rwlockReadLock(RwLock* r);

/**
 * @brief Attempts to lock the read/write lock for reading without waiting.
 * @param r Read/write lock object.
 * @return 1 if the mutex has been acquired successfully, and 0 on contention.
 */
bool rwlockTryReadLock(RwLock* r);

/**
 * @brief Unlocks the read/write lock for reading.
 * @param r Read/write lock object.
 */
void rwlockReadUnlock(RwLock* r);

/**
 * @brief Locks the read/write lock for writing.
 * @param r Read/write lock object.
 */
void rwlockWriteLock(RwLock* r);

/**
 * @brief Attempts to lock the read/write lock for writing without waiting.
 * @param r Read/write lock object.
 * @return 1 if the mutex has been acquired successfully, and 0 on contention.
 */
bool rwlockTryWriteLock(RwLock* r);

/**
 * @brief Unlocks the read/write lock for writing.
 * @param r Read/write lock object.
 */
void rwlockWriteUnlock(RwLock* r);

/**
 * @brief Checks if the write lock is held by the current thread.
 * @param r Read/write lock object.
 * @return 1 if the current hold holds the write lock, and 0 if it does not.
 */
bool rwlockIsWriteLockHeldByCurrentThread(RwLock* r);

/**
 * @brief Checks if the read/write lock is owned by the current thread.
 * @param r Read/write lock object.
 * @return 1 if the current hold holds the write lock or if it holds read locks acquired
 *         while it held the write lock, and 0 if it does not.
 */
bool rwlockIsOwnedByCurrentThread(RwLock* r);
