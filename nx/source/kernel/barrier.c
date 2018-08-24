#include "kernel/barrier.h"

void barrierInit(Barrier* b) {
    mutexInit(&b->mutex);
    mutexLock(&b->mutex);
    if(b->isInited) {
        return;
    }
    listInit(b->threads_registered);
    listInit(b->threads_waiting);
    b->isInited = true;
    mutexUnlock(&b->mutex);
}

void barrierFree(Barrier* b) {
    mutexLock(&b->mutex);
    if(!b->isInited) {
        return;
    }
    listFree(b->threads_registered);
    listFree(b->threads_waiting);
    b->isInited = false;
    mutexUnlock(&b->mutex);
}

void barrierRegister(Barrier* b, Thread* thread) {
    if(listIsInserted(b->threads_registered, (void*)thread)) {
        return;
    }
    listInsertLast(b->threads_registered, (void*)thread);
}

void barrierUnregister(Barrier* b, Thread* thread) {
    listDelete(b->threads_registered, (void*)thread);
}

void barrierWait(Barrier* b, Thread* thread) {
    if(!listIsInserted(b->threads_registered)) {
        return;
    }

    mutexLock(&b->mutex);
    if(listGetNumNodes(b->threads_registered) == listGetNumNodes(b->threads_waiting)+1) {
        while(listGetNumNodes(b->threads_waiting) > 0) {
            Thread* current_thread = listGetItem(b->threads_waiting, 0);
            threadResume(current_thread);
            listDelete(b->threads_waiting, current_thread);
        }
        mutexUnlock(&b->mutex);
    }
    else {
        listInsertLast(b->threads_waiting, thread);
        mutexUnlock(&b->mutex);
        threadPause((void*)thread);
    }
}