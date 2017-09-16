        .text
	.global	initSystem
	.type	initSystem,	%function

initSystem:
	stp x29, x30, [sp, #-16]!
        adr     x1, __nx_binarybase
        str     x0, [x1]
	bl	__libnx_init

	bl	__appInit
	bl	__nx_libc_init_array
	ldp x29, x30, [sp], #16
	ret

	.global	__nx_exit
	.type	__nx_exit,	%function

__nx_exit:
	bl	__nx_libc_fini_array
	bl	__appExit

	b	__libnx_exit

__nx_libc_init_array:
	stp x29, x30, [sp, #-16]!
	stp x21, x22, [sp, #-16]!
	stp x19, x20, [sp, #-16]!
        adr     x3, __nx_binarybase
        ldr     x20, [x3]

        ldr     x0, =__preinit_array_start
        ldr     x1, =__preinit_array_end
        sub     x1, x1, x0
        add     x21, x0, x20
        lsr     x19, x1, #3
        cbz     x19, __nx_libc_init_array_end0

        __nx_libc_init_array_lp0:
        ldr     x3, [x21], #8
        sub     x19, x19, #1
        add     x3, x3, x20
        blr     x3
        cbnz    x19, __nx_libc_init_array_lp0

__nx_libc_init_array_end0:
        bl      _init

        ldr     x0, =__init_array_start
        ldr     x1, =__init_array_end
        sub     x1, x1, x0
        add     x21, x0, x20
        lsr     x19, x1, #3
        cbz     x19, __nx_libc_init_array_end1

        __nx_libc_init_array_lp1:
        ldr     x3, [x21], #8
        sub     x19, x19, #1
        add     x3, x3, x20
        blr     x3
        cbnz    x19, __nx_libc_init_array_lp1

__nx_libc_init_array_end1:
	ldp x19, x20, [sp], #16
	ldp x21, x22, [sp], #16
	ldp x29, x30, [sp], #16
        ret

__nx_libc_fini_array:
	stp x29, x30, [sp, #-16]!
	stp x21, x22, [sp, #-16]!
	stp x19, x20, [sp, #-16]!
        adr     x3, __nx_binarybase
        ldr     x20, [x3]

        ldr     x0, =__fini_array_start
        ldr     x1, =__fini_array_end
        sub     x1, x1, x0
        add     x21, x0, x20
        lsr     x19, x1, #3
        cbz     x19, __nx_libc_fini_array_end

        __nx_libc_fini_array_lp:
        sub     x19, x19, #1
        ldr     x3, [x21, x19, lsl #3]
        add     x3, x3, x20
        blr     x3
        cbnz    x19, __nx_libc_fini_array_lp

__nx_libc_fini_array_end:
	ldp x19, x20, [sp], #16
	ldp x21, x22, [sp], #16
	ldp x29, x30, [sp], #16
        ret

        .data
__nx_binarybase:
        .dword 0

