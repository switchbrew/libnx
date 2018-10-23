#include "kernel/barrier.h"

void barrierInit(Barrier *b, u64 thread_count) {
    b->count = 0;
    b->thread_total = thread_count;
    semaphoreInit(&b->throttle, 0);
    semaphoreInit(&b->lock, 1);
    semaphoreInit(&b->thread_wait, 0);
}

void barrierWait(Barrier *b) {
    semaphoreWait(&b->lock);
    if(b->count < b->thread_total) {
        b->count++;
    }
    if(b->count < b->thread_total) {
        semaphoreSignal(&b->lock);
        semaphoreWait(&b->thread_wait);
        semaphoreSignal(&b->throttle);
    }
    else if(b->count == b->thread_total) {
        for(int i = 0; i < b->thread_total-1; i++) {
            semaphoreSignal(&b->thread_wait);
            semaphoreWait(&b->throttle);
        }
        b->count = 0;
        semaphoreSignal(&b->lock);
    }
}
