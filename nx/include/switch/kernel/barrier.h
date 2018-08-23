/**
 * @file barrier.h
 * @brief Barrier synchronization primitive.
 * @author Yordrar
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"
#include "condvar.h"
#include "mutex.h"
#include "../runtime/util/list.h"

typedef struct barrier {
    List threads_registered;
    List threads_waiting;
    RwLock mutex;
    bool isInited;
} Barrier;

void barrierInit(Barrier* b);
void barrierFree(Barrier* b);
void barrierRegister(Barrier* b, Thread* thread);
void barrierUnregister(Barrier* b, Thread* thread);
void barrierWait(Barrier* b, Thread* thread);
