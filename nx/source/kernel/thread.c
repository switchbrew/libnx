// Copyright 2017 plutoo
#include <switch.h>

static void _EntryWrap(Thread* t) {
    t->entry(t->arg);
    svcExitThread();
}

Result threadCreate(
    Thread* t, ThreadFunc entry, void* arg, size_t stack_sz, int prio,
    int cpuid)
{
    Result rc = 0;
    void*  stack = heapAllocPages(stack_sz);

    if (stack == NULL) {
        rc = MAKERESULT(MODULE_LIBNX, LIBNX_OUTOFMEM);
    }
    else {
        // todo: svcMapMemory returns 0xDC01
        void* stack_mirror = stack;//virtmemReserveMap(stack_sz);
        //rc = svcMapMemory(stack_mirror, stack, stack_sz);

        if (R_SUCCEEDED(rc))
        {
            u64 stack_top = ((u64)stack_mirror) + t->stack_sz;
            Handle handle;

            rc = svcCreateThread(
                &handle, (ThreadFunc) &_EntryWrap, (void*) t, (void*) stack_top,
                prio, cpuid);

            if (R_SUCCEEDED(rc))
            {
                t->handle = handle;
                t->entry = entry;
                t->arg = arg;
                t->stack_mem = stack;
                t->stack_mirror = stack_mirror;
                t->stack_sz = stack_sz;
            }

            if (R_FAILED(rc)) {
                svcUnmapMemory(stack_mirror, stack, stack_sz);
            }
        }

        if (R_FAILED(rc)) {
            virtmemFreeMap(stack_mirror, stack_sz);
            //heapFree(stack);
        }
    }

    return rc;
}

Result threadStart(Thread* t) {
    return svcStartThread(t->handle);
}

Result threadWaitForExit(Thread* t) {
    Handle handle = t->handle;
    s32    idx = 0;
    return svcWaitSynchronization(&idx, &handle, 1, -1);
}

Result threadClose(Thread* t) {
    Result rc;

    rc = svcUnmapMemory(t->stack_mirror, t->stack_mem, t->stack_sz);
    virtmemFreeMap(t->stack_mirror, t->stack_sz);
    //heapFree(t->stack);
    svcCloseHandle(t->handle);

    return rc;
}
