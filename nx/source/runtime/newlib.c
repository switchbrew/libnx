#include <string.h>
#include <errno.h>
#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>
#include "../internal.h"
#include "types.h"
#include "runtime/env.h"
#include "kernel/mutex.h"
#include "services/fatal.h"
#include "services/time.h"
#include "result.h"

void __attribute__((weak)) NORETURN __libnx_exit(int rc);

extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];

/// TimeType passed to timeGetCurrentTime() by __libnx_gtod().
__attribute__((weak)) TimeType __nx_time_type = TimeType_Default;

static struct _reent* __libnx_get_reent(void) {
    ThreadVars* tv = getThreadVars();
    if (tv->magic != THREADVARS_MAGIC)
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadReent));
    return tv->reent;
}

//TODO: timeGetCurrentTime() returns UTC time. How to handle timezones?

int __libnx_gtod(struct _reent *ptr, struct timeval *tp, struct timezone *tz) {
    if (tp != NULL) {
        u64 now=0;
        Result rc=0;

        rc = timeGetCurrentTime(__nx_time_type, &now);
        if (R_FAILED(rc)) {
            ptr->_errno = EINVAL;
            return -1;
        }

        tp->tv_sec =  now;
        tp->tv_usec = now*1000000;//timeGetCurrentTime() only returns seconds.
    }

    if (tz != NULL) {
        tz->tz_minuteswest = 0;
        tz->tz_dsttime = 0;
    }

    return 0;
}

int usleep(useconds_t useconds)
{
    svcSleepThread(useconds * 1000ull);
    return 0;
}

void newlibSetup(void) {
    // Register newlib syscalls
    __syscalls.exit     = __libnx_exit;
    __syscalls.gettod_r = __libnx_gtod;
    __syscalls.getreent = __libnx_get_reent;

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
