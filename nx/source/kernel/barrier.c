#include "kernel/barrier.h"

void barrierInit(barrier *my_barrier, u64 thread_count){
    my_barrier->count = 0;
    my_barrier->thread_total = thread_count;
    semaphoreInit(&my_barrier->throttle, 0);
    semaphoreInit(&my_barrier->lock, 1);
    semaphoreInit(&my_barrier->thread_wait, 0);
}

void barrierWait(barrier *b){
    semaphoreWait(&b->lock);
    if(b->count < b->thread_total){
        b->count++;
    }
    if(b->count < b->thread_total){
        semaphoreSignal(&b->lock);
        semaphoreWait(&b->thread_wait);
        semaphoreSignal(&b->throttle);
    }
    else if(b->count == b->thread_total){
        for(int i = 0; i < b->thread_total-1; i++){
            semaphoreSignal(&b->thread_wait);
            semaphoreWait(&b->throttle);
        }
        b->count = 0;
        semaphoreSignal(&b->lock);
    }
}
