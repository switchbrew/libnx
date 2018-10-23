/**
 * @file barrier.h
 * @brief Multi-threading Barrier
 * @author tatehaga
 * @copyright libnx Authors
 */
#pragma once
#include "mutex.h"
#include "condvar.h"

/// Barrier structure.
typedef struct Barrier {
    u64 count;  ///< Number of threads to reach the barrier.
    u64 total;  ///< Number of threads to wait on.
    Mutex mutex;
    CondVar condvar;
} Barrier;

/**
 * @brief Initializes a barrier and the number of threads to wait on.
 * @param b Barrier object.
 * @param thread_count Initial value for the number of threads the barrier must wait for.
 */
void barrierInit(Barrier *b, u64 thread_count);

/**
 * @brief Forces threads to wait until all threads have called barrierWait.
 * @param b Barrier object.
 */
void barrierWait(Barrier *b);
