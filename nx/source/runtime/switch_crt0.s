.section .crt0, "ax", %progbits
.global _start
.align 2

_start:
    b 1f
    .word __nx_mod0 - _start
    .ascii "HOMEBREW"

.org _start+0x80; 1:
    // Arguments on NSO entry:
    //   x0=zero                  | x1=main thread handle
    // Arguments on NRO entry (homebrew ABI):
    //   x0=ptr to env context    | x1=UINT64_MAX (-1 aka 0xFFFFFFFFFFFFFFFF)
    // Arguments on user-mode exception entry:
    //   x0=excpt type (non-zero) | x1=ptr to excpt context

    // Detect and handle user-mode exceptions first:
    // if (x0 != 0 && x1 != UINT64_MAX) __libnx_exception_entry(<inargs>);
    cmp  x0, #0
    ccmn x1, #1, #4, ne // 4 = Z
    beq  .Lcrt0_main_entry
    b    __libnx_exception_entry

.Lcrt0_main_entry:
    // Preserve registers across function calls
    mov x25, x0  // entrypoint argument 0
    mov x26, x1  // entrypoint argument 1
    mov x27, x30 // loader return address
    mov x28, sp  // initial stack pointer

    // Perform runtime linking on ourselves (including relocations)
    adr  x0, _start    // get aslr base
    adr  x1, __nx_mod0 // get pointer to MOD0 struct
    bl   __nx_dynamic

    // Save initial stack pointer
    adrp x9, __stack_top
    str  x28, [x9, #:lo12:__stack_top]

    // Perform system initialization
    mov  x0, x25
    mov  x1, x26
    mov  x2, x27
    bl   __libnx_init

    // Jump to the main function
    adrp x0, __system_argc // argc
    ldr  w0, [x0, #:lo12:__system_argc]
    adrp x1, __system_argv // argv
    ldr  x1, [x1, #:lo12:__system_argv]
    adrp x30, :got:exit
    ldr  x30, [x30, #:got_lo12:exit]
    b    main

.global __nx_exit
.type   __nx_exit, %function
__nx_exit:
    // Restore stack pointer
    adrp x8, __stack_top
    ldr  x8, [x8, #:lo12:__stack_top]
    mov  sp, x8

    // Jump back to loader
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

    .ascii "LNY1"
    .word  __relro_start        - __nx_mod0
    .word  __data_start         - __nx_mod0

    .ascii "LNY2"
    .word  0x1 // Version/Fix field, increment on recompile-the-worlds as needed
    .word  0x0 // Reserved

.section .bss.__stack_top, "aw", %nobits
.global __stack_top
.align 3

__stack_top:
    .space 8
