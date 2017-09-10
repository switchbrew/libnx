.section ".crt0","ax"
.global _start, _sysexit, main

_start:
    bl startup

    .word 0x454d4f48
    .word 0x57455242

startup:
    // get aslr base
    sub  x28, x30, #4

    // clear .bss
    ldr  x0, =__bss_start__
    ldr  x1, =__bss_end__
    sub  x1, x1, x0  // calculate size
    add  x1, x1, #7  // round up to 8
    bic  x1, x1, #7
    mov  x2, #0
    add  x0, x0, x28 // relocate ptr

bss_loop: 
    str  x2, [x0], #8
    subs x1, x1, #8
    bne  bss_loop

    // relocate .got
    ldr  x0, =__got_start__
    ldr  x1, =__got_end__
    sub  x1, x1, x0  // calculate size
    add  x1, x1, #7  // round up to 8
    bic  x1, x1, #7
    add  x0, x0, x28 // relocate ptr

got_loop: 
    ldr  x2, [x0]
    add  x2, x2, x28
    str  x2, [x0], #8
    subs x1, x1, #8
    bne  got_loop

    ldr  x3, =heapSetup
    add  x3, x3, x28
    blr  x3

    mov  x0, #0 // argc
    mov  x1, #0 // argv

    ldr  x3, =main
    add  x3, x3, x28
    blr  x3

_sysexit:
    svc  0x7
