/**
 * @file barrier.h
 * @brief Barrier synchronization primitive.
 * @author Yordrar
 * @copyright libnx Authors
 */
#pragma once
#include "condvar.h"
#include "mutex.h"
#include "thread.h"
#include "../runtime/util/list.h"

typedef struct barrier {
    List threads_registered;
    List threads_waiting;
    Mutex mutex;
} Barrier;

/**
 * @brief Allocates memory for a barrier
 * @param b Barrier object
 */
void barrierInit(Barrier* b);

/**
 * @brief Frees the memory allocated for a barrier
 * @param b Barrier object
 */
void barrierFree(Barrier* b);

/**
 * @brief Registers a thread that is going to use the specified barrier
 * @param b Barrier object
 * @param thread The thread to register
 */
void barrierRegister(Barrier* b, Thread* thread);

/**
 * @brief Unegisters a thread that is not going to use anymore the specified barrier
 * @param b Barrier object
 * @param thread The thread to unregister
 */
void barrierUnregister(Barrier* b, Thread* thread);

/**
 * @brief Waits until all processes registered in the barrier call this function
 * @param b Barrier object
 */
void barrierWait(Barrier* b);
