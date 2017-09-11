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

SVC_BEGIN svcSetHeapSize
	str x0, [sp, #-16]!
	svc 1
	ldr x2, [sp], #16
	str x1, [x2]
	ret
SVC_END

SVC_BEGIN svcQueryMemory
	str x1, [sp, #-16]!
	svc 0x6
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcCloseHandle
	svc 0x16
	ret
SVC_END

SVC_BEGIN svcCreateTransferMemory
	str x0, [sp, #-16]!
	svc 0x15
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcWaitSynchronization
	str x0, [sp, #-16]!
	svc 0x18
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcConnectToNamedPort
	str x0, [sp, #-16]!
	svc 0x1f
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcSendSyncRequest
	svc 0x21
	ret
SVC_END

SVC_BEGIN svcBreak
	svc 0x26
	ret
SVC_END

SVC_BEGIN svcAcceptSession
	str x0, [sp, #-16]!
	svc 0x41
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcReplyAndReceive
	str x0, [sp, #-16]!
	svc 0x43
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END

SVC_BEGIN svcQueryPhysicalAddress
	str x0, [sp, #-16]!
	svc 0x54
	ldr x2, [sp], #16
	str x1, [x2]
	str x2, [x2, #8]
	str x3, [x2, #16]
	ret
SVC_END

SVC_BEGIN svcQueryIoMapping
	str x0, [sp, #-16]!
	svc 0x55
	ldr x2, [sp], #16
	str x1, [x2]
	ret
SVC_END

SVC_BEGIN svcManageNamedPort
	str x0, [sp, #-16]!
	svc 0x71
	ldr x2, [sp], #16
	str w1, [x2]
	ret
SVC_END
