.macro CODE_BEGIN name
    .section .text.\name, "ax", %progbits
    .global \name
    .type \name, %function
    .align 2
    .cfi_startproc
\name:
.endm

.macro CODE_END
    .cfi_endproc
.endm

// Called by crt0 when the args at the time of entry indicate an exception occured.

.weak __libnx_exception_handler

.weak __libnx_exception_entry
CODE_BEGIN __libnx_exception_entry
    cmp x1, #0
    beq __libnx_exception_entry_abort

    // Abort exception handling when __libnx_exception_handler is not defined.
    adrp x5, :got:__libnx_exception_handler
    ldr x5, [x5, #:got_lo12:__libnx_exception_handler]
    cmp x5, #0
    beq __libnx_exception_entry_abort

    // Load IsCurrentProcessBeingDebugged.

    stp x9, x10, [sp, #-16]!
    stp x11, x12, [sp, #-16]!
    stp x13, x14, [sp, #-16]!
    stp x15, x16, [sp, #-16]!
    stp x17, x18, [sp, #-16]!
    stp x19, x20, [sp, #-16]!
    str x21, [sp, #-16]!

    stp x0, x1, [sp, #-16]!
    sub sp, sp, #16

    mov x0, sp
    mov x1, #8
    mov w2, wzr
    mov x3, #0
    bl svcGetInfo
    mov w6, w0
    ldr x7, [sp], #16

    ldp x0, x1, [sp], #16

    ldr x21, [sp], #16
    ldp x19, x20, [sp], #16
    ldp x17, x18, [sp], #16
    ldp x15, x16, [sp], #16
    ldp x13, x14, [sp], #16
    ldp x11, x12, [sp], #16
    ldp x9, x10, [sp], #16

    // Abort when svcGetInfo failed.
    cbnz w6, __libnx_exception_entry_abort

    // Abort when IsCurrentProcessBeingDebugged is set where __nx_exception_ignoredebug==0.
    adrp x6, __nx_exception_ignoredebug
    ldr  w5, [x6, #:lo12:__nx_exception_ignoredebug]

    cbnz w5, __libnx_exception_entry_start
    cbnz x7, __libnx_exception_entry_abort

__libnx_exception_entry_start:
    adrp x2, __nx_exceptiondump
    add  x2, x2, #:lo12:__nx_exceptiondump
    mov x5, x2

    // error_desc
    str w0, [x2], #4
    // pad
    str wzr, [x2], #4
    str wzr, [x2], #4
    str wzr, [x2], #4

    // GPRs 0..8
    ldp x3, x4, [x1]
    str x5, [x1], #16 // x0 = __nx_exceptiondump
    stp x3, x4, [x2], #16
    ldp x3, x4, [x1], #16
    stp x3, x4, [x2], #16
    ldp x3, x4, [x1], #16
    stp x3, x4, [x2], #16
    ldp x3, x4, [x1], #16
    stp x3, x4, [x2], #16
    ldr x3, [x1], #8
    str x3, [x2], #8

    // GPRs 9..28
    str x9, [x2], #8
    stp x10, x11, [x2], #16
    stp x12, x13, [x2], #16
    stp x14, x15, [x2], #16
    stp x16, x17, [x2], #16
    stp x18, x19, [x2], #16
    stp x20, x21, [x2], #16
    stp x22, x23, [x2], #16
    stp x24, x25, [x2], #16
    stp x26, x27, [x2], #16
    str x28, [x2], #8

    // fp
    str x29, [x2], #8

    // lr
    ldr x3, [x1], #8
    str x3, [x2], #8

    // sp
    adrp x4, __nx_exception_stack
    add  x4, x4, #:lo12:__nx_exception_stack

    adrp x5, __nx_exception_stack_size
    ldr  x5, [x5, #:lo12:__nx_exception_stack_size]
    add x4, x4, x5

    ldr x3, [x1]
    str x4, [x1], #8 // sp = __nx_exception_stack + __nx_exception_stack_size
    str x3, [x2], #8

    // elr_el1 (pc)
    adrp x4, __libnx_exception_returnentry
    add  x4, x4, #:lo12:__libnx_exception_returnentry

    ldr x3, [x1]
    str x4, [x1], #8 // elr_el1 = __libnx_exception_returnentry
    str x3, [x2], #8

    // padding
    str xzr, [x2], #8

    // fpu_gprs
    stp q0, q1, [x2], #32
    stp q2, q3, [x2], #32
    stp q4, q5, [x2], #32
    stp q6, q7, [x2], #32
    stp q8, q9, [x2], #32
    stp q10, q11, [x2], #32
    stp q12, q13, [x2], #32
    stp q14, q15, [x2], #32
    stp q16, q17, [x2], #32
    stp q18, q19, [x2], #32
    stp q20, q21, [x2], #32
    stp q22, q23, [x2], #32
    stp q24, q25, [x2], #32
    stp q26, q27, [x2], #32
    stp q28, q29, [x2], #32
    stp q30, q31, [x2], #32

    // 4 u32s: pstate, afsr0, afsr1, and esr.
    ldr w3, [x1], #4
    str w3, [x2], #4
    ldr w3, [x1], #4
    str w3, [x2], #4
    ldr w3, [x1], #4
    str w3, [x2], #4
    ldr w3, [x1], #4
    str w3, [x2], #4

    //far
    ldr x3, [x1], #8
    str x3, [x2], #8

    mov w0, wzr
    b __libnx_exception_entry_end

__libnx_exception_entry_abort:
    mov w0, #0xf801
__libnx_exception_entry_end:
    bl svcReturnFromException
    b .
CODE_END

// Jumped to by kernel in svcReturnFromException via the overridden elr_el1, with x0 set to __nx_exceptiondump.
CODE_BEGIN __libnx_exception_returnentry
    bl __libnx_exception_handler

    mov w0, wzr
    mov x1, #0
    mov x2, #0
    bl svcBreak
    b .
CODE_END

