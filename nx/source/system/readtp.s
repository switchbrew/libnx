	.section .text.__aarch64_read_tp, "ax", %progbits
	.global __aarch64_read_tp
	.type __aarch64_read_tp, %function
	.align 2
	.cfi_startproc
__aarch64_read_tp:
	mrs x0, tpidrro_el0
	ldr x0, [x0, #0x1F8]
	ret
	.cfi_endproc
