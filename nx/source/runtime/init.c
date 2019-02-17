#include "types.h"
#include "runtime/env.h"
#include "runtime/hosversion.h"
#include "services/sm.h"
#include "services/fatal.h"
#include "services/fs.h"
#include "services/hid.h"
#include "services/time.h"
#include "services/applet.h"
#include "services/set.h"
#include "runtime/devices/fs_dev.h"

void* __stack_top;
void NORETURN __nx_exit(Result rc, LoaderReturnFn retaddr);

void virtmemSetup(void);
void newlibSetup(void);
void argvSetup(void);
void __libnx_init_time(void);

extern u32 __nx_applet_type;

// Must be a multiple of 0x200000.
__attribute__((weak)) size_t __nx_heap_size = 0;

/// Override these with your own if you're using \ref__libnx_exception_handler. __nx_exception_stack is the stack-bottom. Update \ref __nx_exception_stack_size if you change this.
__attribute__((weak)) alignas(16) u8 __nx_exception_stack[0x400];
__attribute__((weak)) u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
/// By default exception handling will be aborted when the current process is being debugged. Set this to non-zero to disable that.
__attribute__((weak)) u32 __nx_exception_ignoredebug = 0;

/// Override this with your own func to handle exceptions. See \ref __nx_exception_stack. See here: https://switchbrew.org/wiki/SVC#Exception_handling
void __attribute__((weak)) __libnx_exception_handler(ThreadExceptionDump *ctx);

ThreadExceptionDump __nx_exceptiondump;

/*
  There are three ways of allocating heap:

    - Normal syscall:

    Allocates heap using |svcSetHeapSize|. The size is provided by a weak symbol
    called |__nx_heap_size|. With the default 0 value, the size is automatically
    determined with svcGetInfo. If running under a process where heap was already
    allocated with svcSetHeapSize, __nx_heap_size should be set manually.

    - Heap override:

    Uses existing heap segment as provided by the homebrew loader environment
    block. This happens automatically if such a setting is provided by the
    homebrew environment.

    - Custom override:

    A program can override the weak symbol |__libnx_initheap| to setup a
    different heap. In this case, the global variables |fake_heap_start|
    and |fake_heap_end| needs to be set appropriately.

    A custom override can be used to implement an "inner heap" located in the .bss
    segment of a process, for example.
 */

void __attribute__((weak)) __libnx_initheap(void)
{
    void*  addr;
    size_t size = 0;
    size_t mem_available = 0, mem_used = 0;

    if (envHasHeapOverride()) {
        addr = envGetHeapOverrideAddr();
        size = envGetHeapOverrideSize();
    }
    else {
        if (__nx_heap_size==0) {
            svcGetInfo(&mem_available, 6, CUR_PROCESS_HANDLE, 0);
            svcGetInfo(&mem_used, 7, CUR_PROCESS_HANDLE, 0);
            if (mem_available > mem_used+0x200000)
                size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
            if (size==0)
                size = 0x2000000*16;
        }
        else {
            size = __nx_heap_size;
        }

        Result rc = svcSetHeapSize(&addr, size);

        if (R_FAILED(rc))
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_HeapAllocFailed));
    }

    // Newlib
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __attribute__((weak)) __nx_win_init(void);
void __attribute__((weak)) userAppInit(void);

void __attribute__((weak)) __appInit(void)
{
    Result rc;

    // Initialize default services.
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    rc = appletInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_AM));

    if (__nx_applet_type != AppletType_None) {
        rc = hidInitialize();
        if (R_FAILED(rc))
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));
    }

    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));

    __libnx_init_time();

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();

    if (&__nx_win_init) __nx_win_init();
    if (&userAppInit) userAppInit();
}

void __attribute__((weak)) userAppExit(void);
void __attribute__((weak)) __nx_win_exit(void);

void __attribute__((weak)) __appExit(void)
{
    if (&userAppExit) userAppExit();
    if (&__nx_win_exit) __nx_win_exit();

    // Cleanup default services.
    fsdevUnmountAll();
    fsExit();
    timeExit();
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

    __nx_exit(0, envGetExitFuncPtr());
}

