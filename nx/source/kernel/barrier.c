#include "kernel/barrier.h"

void barrierInit(Barrier* b) {
    rwlockWriteLock(b->mutex);
    if(b->isInited) {
        return;
    }
    listInit(b->threads_registered);
    listInit(b->threads_waiting);
    b->isInited = true;
    rwlockWriteUnlock(b->mutex);
}

void barrierFree(Barrier* b);

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
    threadPause((void*)thread);
    listInsertLast(b->threads_waiting, thread);

    if(listGetNumNodes(b->threads_registered) == listGetNumNodes(b->threads_waiting)) {
        while(listGetNumNodes(b->threads_waiting) > 0) {
            threadResume(listGetItem(b->threads_waiting, 0));
        }
    }
}