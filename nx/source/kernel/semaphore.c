// Copyright 2018 Kevoot
#include "kernel/semaphore.h"
#include "kernel/svc.h"

void _privSemaphoreWait(Semaphore *sem, bool is_up);

void semaphoreDown(Semaphore *sem) {
    mutexLock(&sem->mutex);
    sem->is_up = false;
    mutexUnlock(&sem->mutex);
}

void semaphoreInit(Semaphore *sem) {
    sem->is_up = false;
    mutexInit(&sem->mutex);
}
            
void semaphoreUp(Semaphore *sem) {
    mutexLock(&sem->mutex);
    sem->is_up = true;
    mutexUnlock(&sem->mutex);
}

void semaphoreWait(Semaphore *sem) {
    _privSemaphoreWait(sem, false);
}

void semaphoreWaitUp(Semaphore *sem) {
    _privSemaphoreWait(sem, true);
}

bool semaphoreIsUp(Semaphore *sem) {
    bool is_up;

    mutexLock(&sem->mutex);
    is_up = sem->is_up;
    mutexUnlock(&sem->mutex);

    return is_up;
}

void _SemaphoreWait(Semaphore *sem, bool is_up) {
    while (true) {
        mutexLock(&sem->mutex);
        
        if (sem->is_up == is_up) {
            mutexUnlock(&sem->mutex);
            break;
        }

        mutexUnlock(&sem->mutex);
        svcSleepThread(1000000);
    }
}
