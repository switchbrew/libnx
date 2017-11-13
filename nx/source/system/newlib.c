#include <switch.h>
#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>
#include "../internal.h"

void __attribute__((weak)) NORETURN __libnx_exit(int rc);

extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];

static struct _reent* __libnx_get_reent() {
    ThreadVars* tv = getThreadVars();
    if (tv->magic != THREADVARS_MAGIC) {
        fatalSimple(MAKERESULT(MODULE_LIBNX, LIBNX_BADREENT));
        for (;;);
    }
    return tv->reent;
}

void newlibSetup() {
    // Register newlib syscalls
    __syscalls.exit     = __libnx_exit;
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

    u32 tls_size = __tdata_lma_end - __tdata_lma;
    if (tls_size)
        memcpy(__tls_start, __tdata_lma, tls_size);
}
