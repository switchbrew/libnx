.section ".crt0","ax"
.global _start

_start:
    b startup
    .word __nx_mod0 - _start
    .ascii "HOMEBREW"

.org _start+0x80
startup:
    // save lr
    mov  x7, x30

    // get aslr base
    bl   +4
    sub  x6, x30, #0x88

    // context ptr and main thread handle
    mov  x5, x0
    mov  x4, x1

    // Handle the exception if needed.
    // if (ctx != NULL && main_thread != -1)__libnx_exception_entry(<inargs>);
    cmp x5, #0
    ccmn x4, #1, #4, ne // 4 = Z
    beq bssclr_start
    b __libnx_exception_entry

bssclr_start:
    mov x27, x7
    mov x25, x5
    mov x26, x4

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
    mov  x0, x6
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
    adrp x30, exit
    add  x30, x30, #:lo12:exit
    b    main

.global __nx_exit
.type   __nx_exit, %function
__nx_exit:
    // restore stack pointer
    adrp x8, __stack_top
    ldr  x8, [x8, #:lo12:__stack_top]
    mov  sp, x8

    // jump back to loader
    br   x1

.global __nx_mod0
__nx_mod0:
    .ascii "MOD0"
    .word  _DYNAMIC             - __nx_mod0
    .word  __bss_start__        - __nx_mod0
    .word  __bss_end__          - __nx_mod0
    .word  __eh_frame_hdr_start - __nx_mod0
    .word  __eh_frame_hdr_end   - __nx_mod0
    .word  0 // "offset to runtime-generated module object" (neither needed, used nor supported in homebrew)

    // MOD0 extensions for homebrew
    .ascii "LNY0"
    .word  __got_start__        - __nx_mod0
    .word  __got_end__          - __nx_mod0
