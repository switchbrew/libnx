/**
 * @file barrier.h
 * @brief Multi-threading Barrier
 * @author tatehaga
 * @copyright libnx Authors
 */
#pragma once
#include "semaphore.h"

// barrier structure
typedef struct Barrier{
    u64 count;
    u64 thread_total;
    Semaphore throttle;
    Semaphore lock;
    Semaphore thread_wait;
} Barrier;

/**
 * @brief Initializes a barrier and the number of threads to wait on.
 * @param b Barrier object.
 * @param thread_count initial value for the number of threads the barrier must wait for.
 */
void barrierInit(Barrier *b, u64 thread_count);

/**
 * @brief Forces threads to wait until all threads have called barrierWait.
 * @param b Barrier object.
 */
void barrierWait(Barrier *b);
