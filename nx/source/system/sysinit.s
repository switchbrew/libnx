        .text
	.global	__nx_init
	.type	__nx_init, %function

__nx_init:
	stp x29, x30, [sp, #-16]!
	bl	__libnx_init
	bl	__appInit
	bl	__libc_init_array
	ldp x29, x30, [sp], #16
	ret

	.global	__nx_exit
	.type	__nx_exit, %function

__nx_exit:
	bl	__libc_fini_array
	bl	__appExit
	b	__libnx_exit
