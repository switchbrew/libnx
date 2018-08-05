// Copyright 2018 Kevoot
#include "kernel/semaphore.h"
#include "kernel/svc.h"

void semaphoreInit(Semaphore *s, u64 initial_count) {
    s->count = initial_count;
    mutexInit(&s->mutex);
    condvarInit(&s->condvar);
}

void semaphoreSignal(Semaphore *s) {
    mutexLock(&s->mutex);
    s->count++;
    condvarWakeOne(&s->condvar);
    mutexUnlock(&s->mutex);
}

void semaphoreWait(Semaphore *s) {
    mutexLock(&s->mutex);
    // Wait until signalled.
    while (!s->count) {
        condvarWait(&s->condvar, &s->mutex);
    }
    s->count--;
    mutexUnlock(&s->mutex);
}

bool semaphoreTryWait(Semaphore *s) {
    mutexLock(&s->mutex);
    bool success = false;
    // Check and immediately return success.
    if (s->count) {
        s->count--;
        success = true;
    }
    mutexUnlock(&s->mutex);
    return success;
}
