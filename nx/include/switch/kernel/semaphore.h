/**
 * @file semaphore.h
 * @brief Thread synchronization based on Mutex.
 * @author Kevoot
 * @copyright libnx Authors
 */
#pragma once

#include "mutex.h"
#include "condvar.h"

typedef struct Semaphore
{
    CondVar condvar;
    Mutex mutex;
    u64 count;
} Semaphore;

void semaphoreInit(Semaphore *, u64);
void semaphoreSignal(Semaphore *);
void semaphoreWait(Semaphore *);
bool semaphoreTryWait(Semaphore *);
