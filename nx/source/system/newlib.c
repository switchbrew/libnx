#include <switch.h>
#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>

void __attribute__((weak)) NORETURN __libnx_exit(int rc);

void newlibSetup() {
    // Register newlib syscalls
    __syscalls.exit = __libnx_exit;

    // Register locking syscalls
    __syscalls.lock_acquire           = mutexLock;
    __syscalls.lock_release           = mutexUnlock;
    __syscalls.lock_acquire_recursive = rmutexLock;
    __syscalls.lock_release_recursive = rmutexUnlock;
}
