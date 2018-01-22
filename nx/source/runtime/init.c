#include "types.h"
#include "runtime/env.h"
#include "services/sm.h"
#include "services/fatal.h"
#include "services/fs.h"
#include "services/hid.h"
#include "services/applet.h"


void __nx_exit(int rc);

void virtmemSetup(void);
void newlibSetup(void);
void argvSetup(void);

extern u32 __nx_applet_type;

// Must be a multiple of 0x2000000.
__attribute__((weak)) size_t __nx_heap_size = 0x2000000*16;

/*
  There are three ways of allocating heap:

    - Normal syscall:

    Allocates heap using |svcSetHeapSize|. The size is provided by a weak symbol
    called |__nx_heap_size|.

    - Heap override:

    Uses existing heap segment as provided by the homebrew loader environment
    block. This happens automatically if such a setting is provided by the
    homebrew environment.

    - Custom override:

    A program can override the weak symbol |__libnx_initheap| to setup a
    different heap. In this case, the global variables |fake_heap_start|
    and |fake_heap_end| needs to be set appropriately.

    A custom override be used to implement an "inner heap" located in the .bss
    segment of a process, for example.
 */

void __attribute__((weak)) __libnx_initheap(void)
{
    void*  addr;
    size_t size;

    if (envHasHeapOverride()) {
        addr = envGetHeapOverrideAddr();
        size = envGetHeapOverrideSize();
    }
    else {
        Result rc = svcSetHeapSize(&addr, __nx_heap_size);
        size = __nx_heap_size;

        if (R_FAILED(rc))
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));
    }

    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __attribute__((weak)) __appInit(void)
{
    Result rc;

    // Initialize default services.
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = appletInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_AM));

    if (__nx_applet_type != AppletType_None) {
        rc = hidInitialize();
        if (R_FAILED(rc))
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));
    }

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

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

void __attribute__((weak)) __libnx_init(void* ctx, Handle main_thread, void* saved_lr)
{
    // Called by crt0.

    // Libnx initialization goes here.
    envSetup(ctx, main_thread, saved_lr);
    newlibSetup();
    virtmemSetup();
    __libnx_initheap();

    // Build argc/argv if present
    argvSetup();

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

    envGetExitFuncPtr()(0);
    while(1);
}
