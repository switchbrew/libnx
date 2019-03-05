// Copyright 2017 plutoo
#include <malloc.h>
#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/virtmem.h"
#include "kernel/mutex.h"
#include "kernel/thread.h"
#include "kernel/wait.h"
#include "services/fatal.h"
#include "../internal.h"

#define NUM_TLS_SLOTS ((0x100 - sizeof(ThreadVars)) / sizeof(void*))

extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];
extern u8 __tls_end[];

static Mutex g_threadMutex;
static Thread* g_threadList;

static u64 g_tlsUsageMask;
static void (* g_tlsDestructors[NUM_TLS_SLOTS])(void*);

// Thread creation args; keep this struct's size 16-byte aligned
typedef struct {
    Thread*        t;
    ThreadFunc     entry;
    void*          arg;
    struct _reent* reent;
    void*          tls;
    void*          padding;
} ThreadEntryArgs;

static void _EntryWrap(ThreadEntryArgs* args) {
    // Initialize thread vars
    ThreadVars* tv = getThreadVars();
    tv->magic      = THREADVARS_MAGIC;
    tv->thread_ptr = args->t;
    tv->reent      = args->reent;
    tv->tls_tp     = (u8*)args->tls-2*sizeof(void*); // subtract size of Thread Control Block (TCB)
    tv->handle     = args->t->handle;

    // Initialize thread info
    mutexLock(&g_threadMutex);
    args->t->tls_array = (void**)((u8*)armGetTls() + 0x100);
    args->t->prev_next = &g_threadList;
    args->t->next = g_threadList;
    if (g_threadList)
        g_threadList->prev_next = &args->t->next;
    g_threadList = args->t;
    mutexUnlock(&g_threadMutex);

    // Launch thread entrypoint
    args->entry(args->arg);
    threadExit();
}

Result threadCreate(
    Thread* t, ThreadFunc entry, void* arg, size_t stack_sz, int prio,
    int cpuid)
{
    stack_sz = (stack_sz+0xFFF) &~ 0xFFF;

    Result rc = 0;
    size_t reent_sz = (sizeof(struct _reent)+0xF) &~ 0xF;
    size_t tls_sz = (__tls_end-__tls_start+0xF) &~ 0xF;
    void*  stack = memalign(0x1000, stack_sz + reent_sz + tls_sz);

    if (stack == NULL) {
        rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }
    else {
        void* stack_mirror = virtmemReserveStack(stack_sz);
        rc = svcMapMemory(stack_mirror, stack, stack_sz);

        if (R_SUCCEEDED(rc))
        {
            u64 stack_top = ((u64)stack_mirror) + stack_sz - sizeof(ThreadEntryArgs);
            ThreadEntryArgs* args = (ThreadEntryArgs*) stack_top;
            Handle handle;

            rc = svcCreateThread(
                &handle, (ThreadFunc) &_EntryWrap, args, (void*)stack_top,
                prio, cpuid);

            if (R_SUCCEEDED(rc))
            {
                t->handle = handle;
                t->stack_mem = stack;
                t->stack_mirror = stack_mirror;
                t->stack_sz = stack_sz;
                t->tls_array = NULL;
                t->next = NULL;
                t->prev_next = NULL;

                args->t = t;
                args->entry = entry;
                args->arg = arg;
                args->reent = (struct _reent*)((u8*)stack + stack_sz);
                args->tls = (u8*)stack + stack_sz + reent_sz;

                // Set up child thread's reent struct, inheriting standard file handles
                _REENT_INIT_PTR(args->reent);
                struct _reent* cur = getThreadVars()->reent;
                args->reent->_stdin  = cur->_stdin;
                args->reent->_stdout = cur->_stdout;
                args->reent->_stderr = cur->_stderr;

                // Set up child thread's TLS segment
                size_t tls_load_sz = __tdata_lma_end - __tdata_lma;
                size_t tls_bss_sz = tls_sz - tls_load_sz;
                if (tls_load_sz)
                    memcpy(args->tls, __tdata_lma, tls_load_sz);
                if (tls_bss_sz)
                    memset(args->tls+tls_load_sz, 0, tls_bss_sz);
            }

            if (R_FAILED(rc)) {
                svcUnmapMemory(stack_mirror, stack, stack_sz);
            }
        }

        if (R_FAILED(rc)) {
            virtmemFreeStack(stack_mirror, stack_sz);
            free(stack);
        }
    }

    return rc;
}

void threadExit(void) {
    Thread* t = getThreadVars()->thread_ptr;
    if (!t)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    u64 tls_mask = __atomic_load_n(&g_tlsUsageMask, __ATOMIC_SEQ_CST);
    for (s32 i = 0; i < NUM_TLS_SLOTS; i ++) {
        if (!(tls_mask & ((UINT64_C(1) << i))))
            continue;
        if (t->tls_array[i]) {
            if (g_tlsDestructors[i])
                g_tlsDestructors[i](t->tls_array[i]);
            t->tls_array[i] = NULL;
        }
    }

    mutexLock(&g_threadMutex);
    *t->prev_next = t->next;
    if (t->next)
        t->next->prev_next = t->prev_next;
    t->tls_array = NULL;
    t->next = NULL;
    t->prev_next = NULL;
    mutexUnlock(&g_threadMutex);

    svcExitThread();
}

Result threadStart(Thread* t) {
    return svcStartThread(t->handle);
}

Result threadWaitForExit(Thread* t) {
    return waitSingleHandle(t->handle, -1);
}

Result threadClose(Thread* t) {
    Result rc;

    if (t->tls_array)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    rc = svcUnmapMemory(t->stack_mirror, t->stack_mem, t->stack_sz);
    virtmemFreeStack(t->stack_mirror, t->stack_sz);
    free(t->stack_mem);
    svcCloseHandle(t->handle);

    return rc;
}

Result threadPause(Thread* t) {
    return svcSetThreadActivity(t->handle, 1);
}

Result threadResume(Thread* t) {
    return svcSetThreadActivity(t->handle, 0);
}

Result threadDumpContext(ThreadContext* ctx, Thread* t) {
    return svcGetThreadContext3(ctx, t->handle);
}

Handle threadGetCurHandle(void) {
    return getThreadVars()->handle;
}

s32 threadTlsAlloc(void (* destructor)(void*)) {
    s32 slot_id;
    u64 new_mask;
    u64 cur_mask = __atomic_load_n(&g_tlsUsageMask, __ATOMIC_SEQ_CST);
    do {
        slot_id = __builtin_ffs(~cur_mask)-1;
        if (slot_id < 0 || slot_id >= NUM_TLS_SLOTS) return -1;
        new_mask = cur_mask | (UINT64_C(1) << slot_id);
    } while (!__atomic_compare_exchange_n(&g_tlsUsageMask, &cur_mask, new_mask, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));

    threadTlsSet(slot_id, NULL);
    mutexLock(&g_threadMutex);
    for (Thread *t = g_threadList; t; t = t->next)
        t->tls_array[slot_id] = NULL;
    mutexUnlock(&g_threadMutex);

    g_tlsDestructors[slot_id] = destructor;
    return slot_id;
}

void* threadTlsGet(s32 slot_id) {
    void** tls_array = (void**)((u8*)armGetTls() + 0x100);
    return tls_array[slot_id];
}

void threadTlsSet(s32 slot_id, void* value) {
    void** tls_array = (void**)((u8*)armGetTls() + 0x100);
    tls_array[slot_id] = value;
}

void threadTlsFree(s32 slot_id) {
    g_tlsDestructors[slot_id] = NULL;

    u64 new_mask;
    u64 cur_mask = __atomic_load_n(&g_tlsUsageMask, __ATOMIC_SEQ_CST);
    do
        new_mask = cur_mask &~ (UINT64_C(1) << slot_id);
    while (!__atomic_compare_exchange_n(&g_tlsUsageMask, &cur_mask, new_mask, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}
