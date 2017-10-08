#include <switch.h>

void __nx_exit(int rc);

void virtmemSetup();
void newlibSetup();

static void _SetupHeap() {
    #define INNER_HEAP_SIZE 0x20000
    #define OUTER_HEAP_SIZE (0x2000000*4)

    char* addr;
    Result rc   = svcSetHeapSize((void**)&addr, OUTER_HEAP_SIZE);
    size_t size = OUTER_HEAP_SIZE;

    if (R_FAILED(rc)) {
        static char g_Heap[INNER_HEAP_SIZE];
        addr = &g_Heap[0];
        size = INNER_HEAP_SIZE;
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
}

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
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
