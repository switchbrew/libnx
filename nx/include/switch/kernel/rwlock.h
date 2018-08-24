/**
 * @file rwlock.h
 * @brief Read/write lock synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../kernel/mutex.h"

/// Read/write lock structure.
typedef struct {
    RMutex r;
    RMutex g;
    u64 b;
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
 * @brief Unlocks the read/write lock for writing.
 * @param r Read/write lock object.
 */
void rwlockWriteUnlock(RwLock* r);
