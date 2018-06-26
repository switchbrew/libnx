// Copyright 2018 Kevoot
#include "kernel/semaphore.h"
#include "kernel/svc.h"

void semaphoreInit(Semaphore *sem, u64 c) {
    sem->count = c;
    mutexInit(&sem->mutex);
    condvarInit(&sem->condvar, &sem->mutex);
}

void semaphoreSignal(Semaphore *sem) {
    mutexLock(&sem->mutex);
    sem->count++;
    condvarWakeOne(&sem->condvar);
    mutexUnlock(&sem->mutex);
}

void semaphoreWait(Semaphore *sem) {
    mutexLock(&sem->mutex);
    while(!(&sem->count)) {
        condvarWait(&sem->condvar);
    }
    sem->count--;
    mutexUnlock(&sem->mutex);
}

bool semaphoreTryWait(Semaphore *sem) {
    mutexLock(&sem->mutex);
    bool success = false;
    if(sem->count) {
        (sem->count)--;
        success = true;
    }
    return success;
}