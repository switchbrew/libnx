#include <switch.h>
#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>

void __attribute__((weak)) NORETURN __libnx_exit(void);


static void NORETURN _ExitImpl(int rc) {
    __libnx_exit();
}

void newlibSetup() {
    void exitImpl(int rc);
    __syscalls.exit = _ExitImpl;

}
