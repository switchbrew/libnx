// Copyright 2018 Kevoot
#include "kernel/semaphore.h"
#include "kernel/svc.h"

void _priv_sem_wait(sem_t *sem, bool flag);

void sem_down(sem_t *sem) {
    mutexLock(&sem->mutex);
    sem->flag = SEM_DOWN;
    mutexUnlock(&sem->mutex);
}

void sem_init(sem_t *sem) {
    sem->flag = SEM_DOWN;
    mutexInit(&sem->mutex);
}

void sem_uninit(sem_t *sem) {
    sem->flag = SEM_DOWN;
}
            
void sem_up(sem_t *sem) {
    mutexLock(&sem->mutex);
    sem->flag = SEM_UP;
    mutexUnlock(&sem->mutex);
}

void sem_wait(sem_t *sem) {
    _priv_sem_wait(sem, SEM_DOWN);
}

void sem_waitup(sem_t *sem) {
    _priv_sem_wait(sem, SEM_UP);
}

bool sem_isup(sem_t *sem) {
    bool flag;

    mutexLock(&sem->mutex);
    flag = sem->flag;
    mutexUnlock(&sem->mutex);

    return flag;
}

void _priv_sem_wait(sem_t *sem, bool flag) {
    while (true) {
        mutexLock(&sem->mutex);
        
        if (sem->flag == flag) {
            mutexUnlock(&sem->mutex);
            break;
        }

        mutexUnlock(&sem->mutex);
        svcSleepThread(1000000);
    }
}