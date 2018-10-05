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
#include "kernel/svc.h"
#include "services/fatal.h"
#include "services/time.h"
#include "result.h"

void __attribute__((weak)) NORETURN __libnx_exit(int rc);

extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];

/// TimeType passed to timeGetCurrentTime() during time initialization. If that fails and __nx_time_type isn't TimeType_Default, timeGetCurrentTime() will be called again with TimeType_Default.
__attribute__((weak)) TimeType __nx_time_type = TimeType_Default;

static struct _reent* __libnx_get_reent(void) {
    ThreadVars* tv = getThreadVars();
    if (tv->magic != THREADVARS_MAGIC)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadReent));
    return tv->reent;
}

static u64 __boottime;
static u64 __bootticks;

// setup boot time variables
void __libnx_init_time(void) {
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

        //Avoid using *printf.
        strncpy(envstr, info.timezoneName, sizeof(envstr)-1);
        strptr = &envstr[strlen(envstr)];
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

int __libnx_clock_getres(clockid_t clock_id, struct timespec *tp) {
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


int __libnx_clock_gettime(clockid_t clock_id, struct timespec *tp) {
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

int __libnx_clock_settime(clockid_t clock_id,const struct timespec *tp) {
    errno = ENOSYS;
    return -1;
}

int __libnx_gtod(struct _reent *ptr, struct timeval *tp, struct timezone *tz) {
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


int __libnx_nanosleep(const struct timespec *req, struct timespec *rem) {
    svcSleepThread(req->tv_sec * 1000000000ull + req->tv_nsec);
    return 0;
}

void newlibSetup(void) {
    // Register newlib syscalls
    __syscalls.exit     = __libnx_exit;
    __syscalls.gettod_r = __libnx_gtod;
    __syscalls.getreent = __libnx_get_reent;

    __syscalls.clock_gettime = __libnx_clock_gettime;
    __syscalls.clock_getres = __libnx_clock_getres;
    __syscalls.clock_settime = __libnx_clock_settime;

    __syscalls.nanosleep = __libnx_nanosleep;


    // Register locking syscalls
    __syscalls.lock_init              = mutexInit;
    __syscalls.lock_acquire           = mutexLock;
    __syscalls.lock_release           = mutexUnlock;
    __syscalls.lock_init_recursive    = rmutexInit;
    __syscalls.lock_acquire_recursive = rmutexLock;
    __syscalls.lock_release_recursive = rmutexUnlock;

    // Initialize thread vars for the main thread
    ThreadVars* tv = getThreadVars();
    tv->magic      = THREADVARS_MAGIC;
    tv->thread_ptr = NULL;
    tv->reent      = _impure_ptr;
    tv->tls_tp     = __tls_start-2*sizeof(void*); // subtract size of Thread Control Block (TCB)
    tv->handle     = envGetMainThreadHandle();

    u32 tls_size = __tdata_lma_end - __tdata_lma;
    if (tls_size)
        memcpy(__tls_start, __tdata_lma, tls_size);
}
