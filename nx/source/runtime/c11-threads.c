#include <stdlib.h>
#include <time.h>
#include <threads.h>
#include "kernel/svc.h"
#include "../internal.h"

#define THRD_MAIN_HANDLE ((thrd_t)~(uintptr_t)0)

static bool timespec_subtract(struct timespec x, struct timespec y, struct timespec *__restrict result)
{
    // Perform the carry for the later subtraction by updating y
    if (x.tv_nsec < y.tv_nsec) {
        int seconds = (y.tv_nsec - x.tv_nsec) / 1000000000 + 1;
        y.tv_nsec -= 1000000000 * seconds;
        y.tv_sec += seconds;
    }
    if (x.tv_nsec - y.tv_nsec > 1000000000) {
        int seconds = (x.tv_nsec - y.tv_nsec) / 1000000000;
        y.tv_nsec += 1000000000 * seconds;
        y.tv_sec -= seconds;
    }

    // Compute the time remaining to wait
    result->tv_sec = x.tv_sec - y.tv_sec;
    result->tv_nsec = x.tv_nsec - y.tv_nsec;

    // Return true if result is negative
    return x.tv_sec < y.tv_sec;
}

static inline u64 impl_timespec2nsec(const struct timespec *__restrict ts)
{
    return (u64)ts->tv_sec * 1000000000 + ts->tv_nsec;
}

static u64 impl_abstimespec2nsec(const struct timespec *__restrict ts)
{
    struct timespec now, diff;
    clock_gettime(CLOCK_REALTIME, &now);
    if (timespec_subtract(*ts, now, &diff))
        return 0;
    return impl_timespec2nsec(&diff);
}

void call_once(once_flag *flag, void (*func)(void))
{
    mtx_lock(&flag->mutex);

    if (flag->status == 0) {
        flag->status = 1;
        mtx_unlock(&flag->mutex);
        func();
        mtx_lock(&flag->mutex);
        flag->status = 2;
        cnd_broadcast(&flag->cond);
    } else {
        while (flag->status == 1)
            cnd_wait(&flag->cond, &flag->mutex);
    }

    mtx_unlock(&flag->mutex);
}

int cnd_broadcast(cnd_t *cond)
{
    if (!cond)
        return thrd_error;

    Result rc = condvarWakeAll(cond);
    return R_SUCCEEDED(rc) ? thrd_success : thrd_error;
}

void cnd_destroy(cnd_t *cond)
{
    // Nothing
}

int cnd_init(cnd_t *cond)
{
    if (!cond)
        return thrd_error;

    condvarInit(cond);
    return thrd_success;
}

int cnd_signal(cnd_t *cond)
{
    if (!cond)
        return thrd_error;

    Result rc = condvarWakeOne(cond);
    return R_SUCCEEDED(rc) ? thrd_success : thrd_error;
}

static int __cnd_timedwait(cnd_t *__restrict cond, mtx_t *__restrict mtx, u64 timeout)
{
    if (!cond || !mtx)
        return thrd_error;

    uint32_t thread_tag_backup = 0;
    if (mtx->type & mtx_recursive) {
        if (mtx->rmutex.counter != 1)
            return thrd_error;
        thread_tag_backup = mtx->rmutex.thread_tag;
        mtx->rmutex.thread_tag = 0;
        mtx->rmutex.counter = 0;
    }

    Result rc = condvarWaitTimeout(cond, &mtx->mutex, timeout);

    if (mtx->type & mtx_recursive) {
        mtx->rmutex.thread_tag = thread_tag_backup;
        mtx->rmutex.counter = 1;
    }

    if (R_VALUE(rc) == KERNELRESULT(TimedOut))
        return thrd_timedout;

    return R_SUCCEEDED(rc) ? thrd_success : thrd_error;
}

int cnd_timedwait(cnd_t *__restrict cond, mtx_t *__restrict mtx, const struct timespec *__restrict abs_time)
{
    if (!abs_time)
        return thrd_error;

    return __cnd_timedwait(cond, mtx, impl_abstimespec2nsec(abs_time));
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
    return __cnd_timedwait(cond, mtx, U64_MAX);
}

void mtx_destroy(mtx_t *mtx)
{
    // Nothing needs to be done here
}

int mtx_init(mtx_t *mtx, int type)
{
    if (!mtx || (type & mtx_timed) || !(type & mtx_plain))
        return thrd_error;

    mtx->type = type;
    if (mtx->type & mtx_recursive)
        rmutexInit(&mtx->rmutex);
    else
        mutexInit(&mtx->mutex);
    return thrd_success;
}

int mtx_lock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;

    if (mtx->type & mtx_recursive)
        rmutexLock(&mtx->rmutex);
    else
        mutexLock(&mtx->mutex);
    return thrd_success;
}

/*
int mtx_timedlock(mtx_t *__restrict mtx, const struct timespec *__restrict ts)
{
}
*/

int mtx_trylock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;

    bool res = false;
    if (mtx->type & mtx_recursive)
        res = rmutexTryLock(&mtx->rmutex);
    else
        res = mutexTryLock(&mtx->mutex);
    return res ? thrd_success : thrd_error;
}

int mtx_unlock(mtx_t *mtx)
{
    if (!mtx)
        return thrd_error;

    if (mtx->type & mtx_recursive)
        rmutexUnlock(&mtx->rmutex);
    else
        mutexUnlock(&mtx->mutex);
    return thrd_success;
}

typedef struct
{
    thrd_start_t func;
    void* arg;

    bool thread_started;
    mtx_t mutex;
    cnd_t cond;
} __thrd_start_info;

static void __thrd_entry(void* __arg)
{
    __thrd_start_info* info = (__thrd_start_info*)__arg;
    thrd_start_t func = info->func;
    void* arg = info->arg;

    mtx_lock(&info->mutex);
    info->thread_started = true;
    cnd_signal(&info->cond);
    mtx_unlock(&info->mutex);

    int rc = func(arg);
    thrd_exit(rc);
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    if (!thr || !func)
        return thrd_error;

    Result rc;
    *thr = NULL;

    u64 core_mask = 0;
    rc = svcGetInfo(&core_mask, 0, CUR_PROCESS_HANDLE, 0);
    if (R_FAILED(rc))
        return thrd_error;

    __thrd_t* t = (__thrd_t*)malloc(sizeof(__thrd_t));
    if (!t)
        return thrd_nomem;

    __thrd_start_info info;
    info.func = func;
    info.arg = arg;
    info.thread_started = false;
    mtx_init(&info.mutex, mtx_plain);
    cnd_init(&info.cond);

    rc = threadCreate(&t->thr, __thrd_entry, &info, 128*1024, 0x3B, -2);
    if (R_FAILED(rc))
        goto _error1;

    rc = svcSetThreadCoreMask(t->thr.handle, -1, core_mask);
    if (R_FAILED(rc))
        goto _error2;

    rc = threadStart(&t->thr);
    if (R_FAILED(rc))
        goto _error2;

    mtx_lock(&info.mutex);
    while (!info.thread_started)
        cnd_wait(&info.cond, &info.mutex);
    mtx_unlock(&info.mutex);

    *thr = t;
    return thrd_success;

_error2:
    threadClose(&t->thr);
_error1:
    free(t);
    return thrd_error;
}

thrd_t thrd_current(void)
{
    Thread* t = getThreadVars()->thread_ptr;
    return t ? (thrd_t)t : THRD_MAIN_HANDLE;
}

/*
int thrd_detach(thrd_t thr)
{
}
*/

int thrd_equal(thrd_t thr1, thrd_t thr2)
{
    return thr1 == thr2;
}

void thrd_exit(int res)
{
    thrd_t t = thrd_current();
    t->rc = res;
    svcExitThread();
}

int thrd_join(thrd_t thr, int *res)
{
    Result rc;
    if (!thr || thr == THRD_MAIN_HANDLE)
        return thrd_error;

    rc = threadWaitForExit(&thr->thr);
    if (R_FAILED(rc))
        return thrd_error;

    if (res)
        *res = thr->rc;

    rc = threadClose(&thr->thr);
    free(thr);

    return R_SUCCEEDED(rc) ? thrd_success : thrd_error;
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    if (!duration)
        return -1;

    svcSleepThread(impl_timespec2nsec(duration));
    if (remaining) {
        remaining->tv_nsec = 0;
        remaining->tv_sec = 0;
    }

    return 0;
}

void thrd_yield(void)
{
    svcSleepThread(-1);
}

/*
int tss_create(tss_t *key, tss_dtor_t dtor)
{
}

void tss_delete(tss_t key)
{
}

void * tss_get(tss_t key)
{
}

int tss_set(tss_t key, void *val)
{
}
*/
