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
    // Get pointer to MOD0 struct (contains offsets to important places)
    adr x28, __nx_mod0

    // Calculate BSS address/size
    ldp  w8, w9, [x28, #8] // load BSS start/end offset from MOD0
    sub  w9, w9, w8        // calculate BSS size
    add  w9, w9, #7        // round up to 8
    bic  w9, w9, #7        // ^
    add  x8, x28, x8       // fixup the start pointer

    // Clear the BSS in 8-byte units
1:  subs w9, w9, #8
    str  xzr, [x8], #8
    bne  1b

    // Preserve registers across function calls
    mov x25, x0  // entrypoint argument 0
    mov x26, x1  // entrypoint argument 1
    mov x27, x30 // loader return address

    // Save initial stack pointer
    mov  x8, sp
    adrp x9, __stack_top
    str  x8, [x9, #:lo12:__stack_top]

    // Parse ELF .dynamic section (which applies relocations to our module)
    adr  x0, _start    // get aslr base
    ldr  w1, [x28, #4] // pointer to .dynamic section
    add  x1, x28, x1
    bl   __nx_dynamic

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
    adrp x30, exit
    add  x30, x30, #:lo12:exit
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

.section .bss.__stack_top, "aw", %nobits
.global __stack_top
.align 3

__stack_top:
    .space 8
