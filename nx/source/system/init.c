#include <switch.h>

#include <sys/iosupport.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/reent.h>
#include <string.h>

void __nx_exit(int rc);

void virtmemSetup();
void heapSetup();

void __attribute__((weak)) __appInit(void)
{
    // Initialize default services.
    smInitialize();
}

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
    smExit();
}

void __attribute__((weak)) __libnx_init(void)
{
    // Called by crt0.

    // Newlib initialization goes here.
    void exitImpl(int rc);
    __syscalls.exit = exitImpl;

    // Libnx initialization goes here.
    virtmemSetup();
    heapSetup();

    // Initialize services.
    __appInit();

    // Call constructors.
    void __libc_init_array(void);
    __libc_init_array();
}

void __attribute__((weak)) NORETURN __libnx_exit(void)
{
    // Call destructors.
    void __libc_fini_array(void);
    __libc_fini_array();

    // Clean up services.
    __appExit();

    svcExitProcess();
    while(1);
}

void NORETURN exitImpl(int rc)
{
    __libnx_exit();
}
