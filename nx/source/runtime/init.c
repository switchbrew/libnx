#include <switch.h>

void __nx_exit(int rc);

void virtmemSetup(void);
void newlibSetup(void);

void __system_initArgv(void);

#define INNER_HEAP_SIZE 0x200000
__attribute__((weak)) size_t __nx_inner_heap_size = INNER_HEAP_SIZE;
__attribute__((weak)) char   __nx_inner_heap[INNER_HEAP_SIZE];
__attribute__((weak)) size_t __nx_outer_heap_size = 0x2000000*16;//Must be a multiple of 0x2000000.

void __attribute__((weak)) __libnx_initheap(void)
{
    u64 addr;
    Result rc   = svcSetHeapSize((void**)&addr, __nx_outer_heap_size);
    size_t size = __nx_outer_heap_size;

    if (R_FAILED(rc)) {
        addr = (u64) &__nx_inner_heap[0];
        size = __nx_inner_heap_size;
    }

    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __attribute__((weak)) __appInit(void)
{
    // Initialize default services.
    smInitialize();
    appletInitialize();
    hidInitialize();

    fsInitialize();
    //fsdevInit();
}

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
    //fsdevExit();
    fsExit();

    hidExit();
    appletExit();
    smExit();
}

void __attribute__((weak)) __libnx_init(void* ctx, Handle main_thread)
{
    // Called by crt0.

    // Libnx initialization goes here.
    envParse(ctx, main_thread);
    newlibSetup();
    virtmemSetup();
    __libnx_initheap();

    // Build argc/argv if present
    __system_initArgv();

    // Initialize services.
    __appInit();

    // Call constructors.
    void __libc_init_array(void);
    __libc_init_array();
}

void __attribute__((weak)) NORETURN __libnx_exit(int rc)
{
    // Call destructors.
    void __libc_fini_array(void);
    __libc_fini_array();

    // Clean up services.
    __appExit();

    svcExitProcess();
    while(1);
}
