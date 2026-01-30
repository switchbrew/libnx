#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>
#include "../internal.h"
#include "types.h"
#include "runtime/env.h"
#include "arm/counter.h"
#include "kernel/mutex.h"
#include "kernel/condvar.h"
#include "kernel/thread.h"
#include "kernel/svc.h"
#include "services/time.h"
#include "runtime/diag.h"
#include "result.h"
#include "alloc.h"

#define THRD_MAIN_HANDLE ((struct __pthread_t*)~(uintptr_t)0)

struct __pthread_t
{
    Thread thr;
    void *rc;
};

void __attribute__((weak)) NX_NORETURN __libnx_exit(int rc);

/// TimeType passed to timeGetCurrentTime() during time initialization. If that fails and __nx_time_type isn't TimeType_Default, timeGetCurrentTime() will be called again with TimeType_Default.
__attribute__((weak)) TimeType __nx_time_type = TimeType_Default;

static inline int errno_from_result(Result res)
{
    switch (R_VALUE(res)) {
        case 0:
            return 0;
        case KERNELRESULT(TimedOut):
            return ETIMEDOUT;
        default:
            return EIO;
    }
}

void __syscall_exit(int rc)
{
    if (&__libnx_exit)
        __libnx_exit(rc);
    for (;;);
}

struct _reent* __syscall_getreent(void)
{
    ThreadVars* tv = getThreadVars();
    if (tv->magic != THREADVARS_MAGIC)
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadReent));
    return tv->reent;
}


void __syscall_lock_acquire(_LOCK_T *lock)
{
    mutexLock(lock);
}

int __syscall_lock_try_acquire(_LOCK_T *lock)
{
    return mutexTryLock(lock) ? 0 : 1;
}

void __syscall_lock_release(_LOCK_T *lock)
{
    mutexUnlock(lock);
}

void __syscall_lock_acquire_recursive(_LOCK_RECURSIVE_T *lock)
{
    rmutexLock(lock);
}

int __syscall_lock_try_acquire_recursive(_LOCK_RECURSIVE_T *lock)
{
    return rmutexTryLock(lock) ? 0 : 1;
}

void __syscall_lock_release_recursive(_LOCK_RECURSIVE_T *lock)
{
    rmutexUnlock(lock);
}

int __syscall_cond_signal(_COND_T *cond)
{
    return errno_from_result(condvarWakeOne(cond));
}

int __syscall_cond_broadcast(_COND_T *cond)
{
    return errno_from_result(condvarWakeAll(cond));
}

int __syscall_cond_wait(_COND_T *cond, _LOCK_T *lock, uint64_t timeout_ns)
{
    return errno_from_result(condvarWaitTimeout(cond, lock, timeout_ns));
}

int __syscall_cond_wait_recursive(_COND_T *cond, _LOCK_RECURSIVE_T *lock, uint64_t timeout_ns)
{
    if (lock->counter != 1)
        return EBADF;

    lock->counter = 0;

    int errcode = errno_from_result(condvarWaitTimeout(cond, &lock->lock, timeout_ns));

    lock->counter = 1;

    return errcode;
}

struct __pthread_t *__syscall_thread_self(void)
{
    Thread* t = getThreadVars()->thread_ptr;
    return t ? (struct __pthread_t *)t : THRD_MAIN_HANDLE;
}

void __syscall_thread_exit(void *value)
{
    struct __pthread_t *thread = __syscall_thread_self();
    if (thread == THRD_MAIN_HANDLE)
        exit(EXIT_FAILURE);

    thread->rc = value;
    threadExit();
}

typedef struct
{
    void* (*func)(void*);
    void* arg;

    bool thread_started;
    Mutex mutex;
    CondVar cond;
} __entry_args;

static void __thread_entry(void* __arg)
{
    __entry_args* info = (__entry_args*)__arg;
    void* (*func)(void*) = info->func;
    void* arg = info->arg;

    mutexLock(&info->mutex);
    info->thread_started = true;
    condvarWakeOne(&info->cond);
    mutexUnlock(&info->mutex);

    void* rc = func(arg);
    __syscall_thread_exit(rc);
}

int __syscall_thread_create(struct __pthread_t **thread, void* (*func)(void*), void *arg, void *stack_addr, size_t stack_size)
{
    if (((uintptr_t)stack_addr & 0xFFF) || (stack_size & 0xFFF))
        return EINVAL;
    if (!stack_size)
        stack_size = 128*1024;

    Result rc;
    *thread = NULL;

    u64 core_mask = 0;
    rc = svcGetInfo(&core_mask, InfoType_CoreMask, CUR_PROCESS_HANDLE, 0);
    if (R_FAILED(rc))
        return EPERM;

    struct __pthread_t* t = (struct __pthread_t*)__libnx_alloc(sizeof(struct __pthread_t));
    if (!t)
        return ENOMEM;

    __entry_args info;
    info.func = func;
    info.arg = arg;
    info.thread_started = false;
    mutexInit(&info.mutex);
    condvarInit(&info.cond);

    rc = threadCreate(&t->thr, __thread_entry, &info, stack_addr, stack_size, 0x3B, -2);
    if (R_FAILED(rc))
        goto _error1;

    rc = svcSetThreadCoreMask(t->thr.handle, -1, core_mask);
    if (R_FAILED(rc))
        goto _error2;

    rc = threadStart(&t->thr);
    if (R_FAILED(rc))
        goto _error2;

    mutexLock(&info.mutex);
    while (!info.thread_started)
        condvarWait(&info.cond, &info.mutex);
    mutexUnlock(&info.mutex);

    *thread = t;
    return 0;

_error2:
    threadClose(&t->thr);
_error1:
    __libnx_free(t);
    return ENOMEM;
}

void* __syscall_thread_join(struct __pthread_t *thread)
{
    if (thread == THRD_MAIN_HANDLE)
        return NULL;

    Result rc = threadWaitForExit(&thread->thr);
    if (R_FAILED(rc))
        return NULL;

    void* ret = thread->rc;
    threadClose(&thread->thr);
    __libnx_free(thread);

    return ret;
}

/* Unsupported
int __syscall_thread_detach(struct __pthread_t *thread)
{
}
*/

int __syscall_tls_create(uint32_t *key, void (*destructor)(void*))
{
    s32 slot_id = threadTlsAlloc(destructor);
    if (slot_id >= 0) {
        *key = slot_id;
        return 0;
    }

    return EAGAIN;
}

int __syscall_tls_set(uint32_t key, const void *value)
{
    threadTlsSet(key, (void*)value);
    return 0;
}

void* __syscall_tls_get(uint32_t key)
{
    return threadTlsGet(key);
}

int __syscall_tls_delete(uint32_t key)
{
    threadTlsFree(key);
    return 0;
}

int sched_yield(void)
{
    svcSleepThread(-1);
    return 0;
}

int sched_getcpu(void)
{
    return svcGetCurrentProcessorNumber();
}

static u64 __boottime;
static u64 __bootticks;

// setup boot time variables
void __libnx_init_time(void)
{
    TimeCalendarAdditionalInfo info;
    char envstr[64];
    char *strptr;
    bool is_west=0;
    s32 tmp_offset, hour, minute, second;

    Result rc =  timeGetCurrentTime(__nx_time_type, &__boottime);
    if (R_FAILED(rc) && __nx_time_type != TimeType_Default) rc =  timeGetCurrentTime(TimeType_Default, &__boottime);
    if (R_FAILED(rc)) {
        __boottime = UINT64_MAX;
    } else {
        __bootticks = armGetSystemTick();
    }

    if (R_SUCCEEDED(rc)) rc = timeToCalendarTimeWithMyRule(__boottime, NULL, &info);

    if (R_SUCCEEDED(rc)) {
        info.timezoneName[7] = 0;
        tmp_offset = info.offset;
        if (tmp_offset < 0) {
            is_west = 1;
            tmp_offset = -tmp_offset;
        }

        second = tmp_offset % 60;
        tmp_offset /= 60;
        minute = tmp_offset % 60;
        tmp_offset /= 60;
        hour = tmp_offset % 24;

        memset(envstr, 0, sizeof(envstr));
        strptr = envstr;
         // Some tznames have numeric characters and '-'/'+', so quote tzname with <>.
        *strptr++ = '<';
        //Avoid using *printf.
        strncpy(strptr, info.timezoneName, sizeof(envstr)-1);
        strptr = &envstr[strlen(envstr)];
        *strptr++ = '>';
        *strptr++ = is_west ? '+' : '-';

        *strptr++ = '0' + (hour / 10);
        *strptr++ = '0' + (hour % 10);
        *strptr++ = ':';

        *strptr++ = '0' + (minute / 10);
        *strptr++ = '0' + (minute % 10);
        *strptr++ = ':';

        *strptr++ = '0' + (second / 10);
        *strptr++ = '0' + (second % 10);

        setenv("TZ", envstr, 0);
    }
}

static const u64 nsec_clockres =  1000000000ULL / 19200000ULL;

int __syscall_clock_getres(clockid_t clock_id, struct timespec *tp)
{
    if(clock_id != CLOCK_MONOTONIC && clock_id != CLOCK_REALTIME) {
        errno = EINVAL;
        return -1;
    }
    if(tp) {
        tp->tv_sec = 0;
        tp->tv_nsec = nsec_clockres;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

int __syscall_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if(clock_id != CLOCK_MONOTONIC && clock_id != CLOCK_REALTIME) {
        errno = EINVAL;
        return -1;
    }
    if(tp) {

        if(__boottime == UINT64_MAX) {
            errno = EIO;
            return -1;
        }

        u64 now=armGetSystemTick() - __bootticks;

        u64 __bootsecs = now / 19200000ULL;

        tp->tv_sec =  __bootsecs + __boottime;
        u64 nsecs = (now -  __bootsecs * 19200000ULL) * 10000ULL / 192ULL;
        tp->tv_nsec = nsecs - nsecs % nsec_clockres;
        return 0;
    } else {
        errno = EFAULT;
        return -1;
    }
}

int __syscall_gettod_r(struct _reent *ptr, struct timeval *tp, struct timezone *tz)
{
    if (tp != NULL) {

        if(__boottime == UINT64_MAX) {
            ptr->_errno = EIO;
            return -1;
        }

        u64 now=armGetSystemTick() - __bootticks;

        u64 __bootsecs = now / 19200000ULL;

        tp->tv_sec =  __bootsecs + __boottime;
        tp->tv_usec = (now -  __bootsecs * 19200000ULL) * 10ULL / 192ULL;
    }

    if (tz != NULL) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }

    return 0;
}

int __syscall_nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (!req) {
        errno = EINVAL;
        return -1;
    }

    svcSleepThread(timespec2nsec(req));
    if (rem) {
        rem->tv_nsec = 0;
        rem->tv_sec = 0;
    }
    return 0;
}

void newlibSetup(void)
{
    // Initialize thread vars for the main thread
    ThreadVars* tv = getThreadVars();
    tv->magic      = THREADVARS_MAGIC;
    tv->thread_ptr = NULL;
    tv->reent      = _impure_ptr;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    tv->tls_tp     = __tls_start-getTlsStartOffset();
#pragma GCC diagnostic pop
    tv->handle     = envGetMainThreadHandle();

    u32 tls_size = __tdata_lma_end - __tdata_lma;
    if (tls_size)
        memcpy(__tls_start, __tdata_lma, tls_size);
}
