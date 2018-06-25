// Copyright 2018 Kevoot

#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#include "mutex.h"

#define SEM_DOWN false
#define SEM_UP   true

#define EBUSY 1

typedef struct sem_t
{
    bool flag;
    Mutex mutex;
} sem_t;

void sem_init(sem_t *);
void sem_uninit(sem_t *);
/* TODO */
void set_post(sem_t *);
void sem_up(sem_t *);
void sem_down(sem_t *);
void sem_wait(sem_t *);
void sem_waitup(sem_t *);
bool sem_isup(sem_t *);

#endif