#include "kernel/barrier.h"
#include "../internal.h"

void barrierInit(Barrier* b) {
    mutexInit(&b->mutex);
    mutexLock(&b->mutex);
    listInit(&b->threads_registered);
    listInit(&b->threads_waiting);
    mutexUnlock(&b->mutex);
}

void barrierFree(Barrier* b) {
    mutexLock(&b->mutex);
    listFree(&b->threads_registered);
    listFree(&b->threads_waiting);
    mutexUnlock(&b->mutex);
}

void barrierRegister(Barrier* b, Thread* thread) {
    if(listIsInserted(&b->threads_registered, (void*)thread)) {
        return;
    }
    listInsertLast(&b->threads_registered, (void*)thread);
}

void barrierUnregister(Barrier* b, Thread* thread) {
    listDelete(&b->threads_registered, (void*)thread);
}

void barrierWait(Barrier* b) {
    Thread* thread = getThreadVars()->thread_ptr;
    if(!listIsInserted(&b->threads_registered, (void*)thread)) {
        return;
    }

    mutexLock(&b->mutex);
    if(listGetNumNodes(&b->threads_registered) == listGetNumNodes(&b->threads_waiting)+1) {
        while(listGetNumNodes(&b->threads_waiting) > 0) {
            Thread* current_thread = listGetItem(&b->threads_waiting, 0);
            svcSetThreadActivity(current_thread->handle, 0);
            listDelete(&b->threads_waiting, (void*)current_thread);
        }
        mutexUnlock(&b->mutex);
    }
    else {
        listInsertLast(&b->threads_waiting, (void*)thread);
        mutexUnlock(&b->mutex);
        svcSleepThread(0);
    }
}