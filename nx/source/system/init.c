#include <switch.h>

void __nx_exit(int rc);

void virtmemSetup();
void newlibSetup();

#define INNER_HEAP_SIZE 0x20000
__attribute__((weak)) size_t __nx_inner_heap_size = INNER_HEAP_SIZE;
__attribute__((weak)) char __nx_inner_heap[INNER_HEAP_SIZE];
__attribute__((weak)) size_t __nx_outer_heap_size = 0x2000000*4;//Must be a multiple of 0x2000000.

static void _SetupHeap() {
    char* addr;
    Result rc   = svcSetHeapSize((void**)&addr, __nx_outer_heap_size);
    size_t size = __nx_outer_heap_size;

    if (R_FAILED(rc)) {
        addr = &__nx_inner_heap[0];
        size = __nx_inner_heap_size;
    }

    // Multilib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = addr;
    fake_heap_end   = addr + size;
}

void __attribute__((weak)) __appInit(void)
{
    // Initialize default services.
    smInitialize();
    fsInitialize();
}

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
    fsExit();
    smExit();
}

void __attribute__((weak)) __libnx_init(void)
{
    // Called by crt0.

    // Libnx initialization goes here.
    newlibSetup();
    virtmemSetup();
    _SetupHeap();

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
