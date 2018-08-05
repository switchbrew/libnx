/**
 * @file semaphore.h
 * @brief Thread synchronization based on Mutex.
 * @author SciresM & Kevoot
 * @copyright libnx Authors
 */
#pragma once

#include "mutex.h"
#include "condvar.h"

/// Semaphore structure.
typedef struct Semaphore
{
    CondVar condvar; ///< Condition variable object.
    Mutex   mutex;   ///< Mutex object.
    u64     count;   ///< Internal counter.
} Semaphore;

/**
 * @brief Initializes a semaphore and its internal counter.
 * @param s Semaphore object.
 * @param initial_count initial value for internal counter (typically the # of free resources).
 */
void semaphoreInit(Semaphore *s, u64 initial_count);

/**
 * @brief Increments the Semaphore to allow other threads to continue.
 * @param s Semaphore object.
 */
void semaphoreSignal(Semaphore *s);

/**
 * @brief Decrements Semaphore and waits if 0.
 * @param s Semaphore object.
 */
void semaphoreWait(Semaphore *s);

/**
 * @brief Attempts to get lock without waiting.
 * @param s Semaphore object.
 * @return true if no wait and successful lock, false otherwise.
 */
bool semaphoreTryWait(Semaphore *s);
