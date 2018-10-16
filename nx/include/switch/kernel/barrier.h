#pragma once
#include "semaphore.h"


// barrier structure
typedef struct barrier{
    u64 count;
    u64 thread_total;
    Semaphore throttle;
    Semaphore lock;
    Semaphore thread_wait;
} barrier;

void barrierInit(barrier *my_barrier, u64 thread_count);

void barrierWait(barrier *b);
