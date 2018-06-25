/**
 * @file semaphore.h
 * @brief Thread synchronization based on Mutex.
 * @author Kevoot
 * @copyright libnx Authors
 */
#pragma once

#include "mutex.h"

typedef struct Semaphore
{
    bool is_up;
    Mutex mutex;
} Semaphore;

void semaphoreInit(Semaphore *);
/* TODO */
void semaphorePost(Semaphore *);
void semaphoreUp(Semaphore *);
void semaphoreDown(Semaphore *);
void semaphoreWait(Semaphore *);
void semaphoreWaitUp(Semaphore *);
bool semaphoreIsUp(Semaphore *);
