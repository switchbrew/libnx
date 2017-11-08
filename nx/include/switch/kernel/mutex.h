// Copyright 2017 plutoo
#pragma once
#include <sys/lock.h>

typedef _LOCK_T Mutex;
typedef _LOCK_RECURSIVE_T RMutex;

void mutexLock(Mutex* m);
void mutexUnlock(Mutex* m);

void rmutexLock(RMutex* m);
void rmutexUnlock(RMutex* m);
