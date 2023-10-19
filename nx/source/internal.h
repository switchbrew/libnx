#pragma once
#include "types.h"
#include "arm/tls.h"
#include "kernel/thread.h"

#define THREADVARS_MAGIC 0x21545624 // !TV$

// This structure is exactly 0x20 bytes
typedef struct {
    // Magic value used to check if the struct is initialized
    u32 magic;

    // Thread handle, for mutexes
    Handle handle;

    // Pointer to the current thread (if exists)
    Thread* thread_ptr;

    // Pointer to this thread's newlib state
    struct _reent* reent;

    // Pointer to this thread's thread-local segment
    void* tls_tp; // !! Offset needs to be TLS+0x1F8 for __aarch64_read_tp !!
} ThreadVars;

extern const u8 __tdata_lma[];
extern const u8 __tdata_lma_end[];
extern u8 __tls_start[];
extern u8 __tls_end[];
extern size_t __tls_align;

static inline ThreadVars* getThreadVars(void) {
    return (ThreadVars*)((u8*)armGetTls() + 0x200 - sizeof(ThreadVars));
}

NX_INLINE size_t getTlsStartOffset(void)
{
    // TLS region begins with the Thread Control Block (TCB), which is intended
    // to contain two pointers. The actual tdata/tbss segment follows the TCB,
    // however if it requires special alignment the offset is rounded up.
    size_t tcb_sz = 2*sizeof(void*);
    return __tls_align > tcb_sz ? __tls_align : tcb_sz;
}
