.section ".crt0","ax"
.global _start

_start:
    b startup
    .word 0
    .ascii "HOMEBREW"

.org _start+0x80
startup:
    // save lr
    mov  x27, x30

    // get aslr base
    bl   +4
    sub  x28, x30, #0x88

    // context ptr and main thread handle
    mov  x25, x0
    mov  x26, x1

    // clear .bss
    adrp x0, __bss_start__
    adrp x1, __bss_end__
    add  x0, x0, #:lo12:__bss_start__
    add  x1, x1, #:lo12:__bss_end__
    sub  x1, x1, x0  // calculate size
    add  x1, x1, #7  // round up to 8
    bic  x1, x1, #7

bss_loop: 
    str  xzr, [x0], #8
    subs x1, x1, #8
    bne  bss_loop

    // store stack pointer
    mov  x1, sp
    adrp x0, __stack_top
    str  x1, [x0, #:lo12:__stack_top]

    // process .dynamic section
    mov  x0, x28
    adrp x1, _DYNAMIC
    add  x1, x1, #:lo12:_DYNAMIC
    bl   __nx_dynamic

    // initialize system
    mov  x0, x25
    mov  x1, x26
    mov  x2, x27
    bl   __libnx_init

    // call entrypoint
    adrp x0, __system_argc // argc
    ldr  w0, [x0, #:lo12:__system_argc]
    adrp x1, __system_argv // argv
    ldr  x1, [x1, #:lo12:__system_argv]
    adrp x30, __libnx_exit
    add  x30, x30, #:lo12:__libnx_exit
    b    main

.global __nx_exit
.type   __nx_exit, %function
__nx_exit:
    mov  x8, x0

    // restore stack pointer
    adrp x0, __stack_top
    ldr  x0, [x0, #:lo12:__stack_top]
    mov  sp, x0

    // jump back to loader
    mov  x0, #0
    br   x8
