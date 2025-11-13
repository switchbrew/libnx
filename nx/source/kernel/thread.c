// Copyright 2017 plutoo
#include <string.h>
#include "types.h"
#include "result.h"
#include "kernel/svc.h"
#include "kernel/virtmem.h"
#include "kernel/mutex.h"
#include "kernel/thread.h"
#include "kernel/wait.h"
#include "runtime/env.h"
#include "runtime/diag.h"
#include "../internal.h"
#include "../runtime/alloc.h"

#define USER_TLS_BEGIN 0x180
#define USER_TLS_END   (0x200 - sizeof(ThreadVars))
#define NUM_TLS_SLOTS ((USER_TLS_END - USER_TLS_BEGIN) / sizeof(void*))

static Mutex g_threadMutex;
static Thread* g_threadList;

static Thread g_mainThread;

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
    tv->tls_tp     = (u8*)args->tls-getTlsStartOffset();
    tv->handle     = args->t->handle;

    // Initialize thread info
    mutexLock(&g_threadMutex);
    args->t->tls_array = (void**)((u8*)armGetTls() + USER_TLS_BEGIN);
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

void __libnx_init_thread(void) {
    g_mainThread.handle         = envGetMainThreadHandle();

    MemoryInfo mem_info = {0};
    u32 page_info;
    svcQueryMemory(&mem_info, &page_info, (u64)(&mem_info));

    // Set stack.
    g_mainThread.owns_stack_mem = false;
    g_mainThread.stack_mem      = NULL;
    g_mainThread.stack_mirror   = (void*)mem_info.addr;
    g_mainThread.stack_sz       = mem_info.size;

    // Set the TLS array.
    mutexLock(&g_threadMutex);
    g_mainThread.tls_array = (void**)((u8*)armGetTls() + USER_TLS_BEGIN);
    g_mainThread.prev_next = &g_threadList;
    g_mainThread.next = g_threadList;
    if (g_threadList)
        g_threadList->prev_next = &g_mainThread.next;
    g_threadList = &g_mainThread;
    mutexUnlock(&g_threadMutex);

    // Set thread_ptr.
    getThreadVars()->thread_ptr = &g_mainThread;
}

Result threadCreate(
    Thread* t, ThreadFunc entry, void* arg, void* stack_mem, size_t stack_sz,
    int prio, int cpuid)
{
    const size_t reent_sz = (sizeof(struct _reent)+0xF) &~ 0xF;
    const size_t tls_sz = (__tls_end-__tls_start+0xF) &~ 0xF;

    // Verify stack size alignment
    if (stack_sz & 0xFFF) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    bool owns_stack_mem;
    if (stack_mem == NULL) {
        // Allocate new memory for the stack, tls and reent.
        stack_mem = __libnx_aligned_alloc(0x1000, stack_sz + tls_sz + reent_sz);

        owns_stack_mem = true;
    } else {
        // Verify alignment of provided memory.
        if ((uintptr_t)stack_mem & 0xFFF) {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        }

        // Ensure we don't go out of bounds.
        size_t align_mask = getTlsStartOffset()-1;
        size_t needed_sz = (tls_sz + reent_sz + align_mask) &~ align_mask;
        if (stack_sz <= needed_sz + sizeof(ThreadEntryArgs)) {
            return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        }

        // Use provided memory for the stack, tls and reent.
        stack_sz -= needed_sz;
        owns_stack_mem = false;
    }

    if (stack_mem == NULL) {
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
    }

    // Total allocation size may be unaligned in either case.
    virtmemLock();
    const size_t aligned_stack_sz = (stack_sz + tls_sz + reent_sz + 0xFFF) & ~0xFFF;
    void* stack_mirror = virtmemFindStack(aligned_stack_sz, 0x4000);
    Result rc = svcMapMemory(stack_mirror, stack_mem, aligned_stack_sz);
    virtmemUnlock();

    if (R_SUCCEEDED(rc))
    {
        uintptr_t stack_top = (uintptr_t)stack_mirror + stack_sz - sizeof(ThreadEntryArgs);
        ThreadEntryArgs* args = (ThreadEntryArgs*) stack_top;
        void *tls = (void*)((uintptr_t)stack_mirror + stack_sz);
        void *reent = (void*)((uintptr_t)tls + tls_sz);

        Handle handle;

        t->handle = INVALID_HANDLE;
        t->owns_stack_mem = owns_stack_mem;
        t->stack_mem = stack_mem;
        t->stack_mirror = stack_mirror;
        t->stack_sz = stack_sz - sizeof(ThreadEntryArgs);
        t->tls_array = NULL;
        t->next = NULL;
        t->prev_next = NULL;

        args->t = t;
        args->entry = entry;
        args->arg = arg;
        args->reent = reent;
        args->tls = tls;

        rc = svcCreateThread(
            &handle, (ThreadFunc) &_EntryWrap, args, (void*)stack_top,
            prio, cpuid);

        if (R_SUCCEEDED(rc))
        {
            t->handle = handle;

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
            svcUnmapMemory(stack_mirror, stack_mem, aligned_stack_sz);
        }
    }

    if (R_FAILED(rc)) {
        if (owns_stack_mem) {
            __libnx_free(stack_mem);
        }
    }

    return rc;
}

void threadExit(void) {
    Thread* t = getThreadVars()->thread_ptr;
    if (!t)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_NotInitialized));

    u64 tls_mask = __atomic_load_n(&g_tlsUsageMask, __ATOMIC_SEQ_CST);
    for (s32 i = 0; i < NUM_TLS_SLOTS; i ++) {
        if (!(tls_mask & ((UINT64_C(1) << i))))
            continue;
        if (t->tls_array[i]) {
            void* old_value = t->tls_array[i];
            t->tls_array[i] = NULL;
            if (g_tlsDestructors[i])
                g_tlsDestructors[i](old_value);
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

    const size_t tls_sz = (__tls_end-__tls_start+0xF) &~ 0xF;
    const size_t reent_sz = (sizeof(struct _reent)+0xF) &~ 0xF;
    const size_t aligned_stack_sz = (t->stack_sz + sizeof(ThreadEntryArgs) + tls_sz + reent_sz + 0xFFF) & ~0xFFF;

    rc = svcUnmapMemory(t->stack_mirror, t->stack_mem, aligned_stack_sz);

    if (R_SUCCEEDED(rc)) {
        if (t->owns_stack_mem) {
            __libnx_free(t->stack_mem);
        }
        svcCloseHandle(t->handle);
    }

    return rc;
}

Result threadPause(Thread* t) {
    return svcSetThreadActivity(t->handle, ThreadActivity_Paused);
}

Result threadResume(Thread* t) {
    return svcSetThreadActivity(t->handle, ThreadActivity_Runnable);
}

Result threadDumpContext(ThreadContext* ctx, Thread* t) {
    return svcGetThreadContext3(ctx, t->handle);
}

Thread *threadGetSelf(void) {
    return getThreadVars()->thread_ptr;
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
    void** tls_array = (void**)((u8*)armGetTls() + USER_TLS_BEGIN);
    return tls_array[slot_id];
}

void threadTlsSet(s32 slot_id, void* value) {
    void** tls_array = (void**)((u8*)armGetTls() + USER_TLS_BEGIN);
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
