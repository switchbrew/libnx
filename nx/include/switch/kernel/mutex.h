// Copyright 2017 plutoo
#pragma once
#include <sys/lock.h>
#include <switch/types.h> // not needed in this file, still including it

typedef _LOCK_T Mutex;
typedef _LOCK_RECURSIVE_T RMutex;

static inline void mutexInit(Mutex* m)
{
    *m = 0;
}

void mutexLock(Mutex* m);
void mutexUnlock(Mutex* m);

static inline void rmutexInit(RMutex* m)
{
    m->lock = 0;
    m->thread_tag = 0;
    m->counter = 0;
}

void rmutexLock(RMutex* m);
void rmutexUnlock(RMutex* m);
