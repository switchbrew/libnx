	.global	initSystem
	.type	initSystem,	%function

initSystem:
	stp x29, x30, [sp, #-16]!
	bl	__libnx_init

	bl	__appInit
	//bl	__libc_init_array //Currently broken since this doesn't use .got.
	ldp x29, x30, [sp], #16
	ret

	.global	__nx_exit
	.type	__nx_exit,	%function

__nx_exit:
	//bl	__libc_fini_array//See above.
	bl	__appExit

	b	__libnx_exit

