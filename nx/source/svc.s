.macro SVC_BEGIN name
	.section .text.\name, "ax", %progbits
	.global \name
	.type \name, %function
	.align 2
	.cfi_startproc
\name:
.endm

.macro SVC_END
	.cfi_endproc
.endm

SVC_BEGIN svcReplyAndReceive
	str x0, [sp, #-16]!
	svc  0x43
	ldr  x2, [sp], #16
	str  w1, [x2]
	ret
SVC_END

SVC_BEGIN svcQueryIoMapping
	str x0, [sp, #-16]!
	svc  0x43
	ldr  x2, [sp], #16
	str  x1, [x2]
	ret
SVC_END

SVC_BEGIN svcManageNamedPort
	str x0, [sp, #-16]!
	svc  0x71
	ldr  x2, [sp], #16
	str  w1, [x2]
	ret
SVC_END

