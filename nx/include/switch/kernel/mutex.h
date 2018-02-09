/**
 * @file mutex.h
 * @brief Mutex synchronization primitive.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include <sys/lock.h>
#include "../types.h" // not needed in this file, still including it

/// Mutex datatype, defined in newlib.
typedef _LOCK_T Mutex;
/// Recursive mutex datatype, defined in newlib.
typedef _LOCK_RECURSIVE_T RMutex;

/**
 * @brief Initializes a mutex.
 * @param m Mutex object.
 */
static inline void mutexInit(Mutex* m)
{
    *m = 0;
}

/**
 * @brief Locks a mutex.
 * @param m Mutex object.
 */
void mutexLock(Mutex* m);

/**
 * @brief Unlocks a mutex.
 * @param m Mutex object.
 */
void mutexUnlock(Mutex* m);

/**
 * @brief Initializes a recursive mutex.
 * @param m Recursive mutex object.
 */
static inline void rmutexInit(RMutex* m)
{
    m->lock = 0;
    m->thread_tag = 0;
    m->counter = 0;
}

/**
 * @brief Locks a recursive mutex.
 * @param m Recursive mutex object.
 */
void rmutexLock(RMutex* m);

/**
 * @brief Unlocks a recursive mutex.
 * @param m Recursive mutex object.
 */
void rmutexUnlock(RMutex* m);
