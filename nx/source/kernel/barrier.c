#include "kernel/barrier.h"

void barrierInit(Barrier *b, u64 total) {
    b->count = 0;
    b->total = total - 1;
    mutexInit(&b->mutex);
    condvarInit(&b->condvar);
}

void barrierWait(Barrier *b) {
    mutexLock(&b->mutex);

    if (b->count++ == b->total) {
        b->count = 0;
        condvarWake(&b->condvar, b->total);
    }
    else {
        condvarWait(&b->condvar, &b->mutex);
    }

    mutexUnlock(&b->mutex);
}
