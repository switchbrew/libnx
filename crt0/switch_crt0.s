	.section ".crt0","ax"
	.global _start, _sysexit, main
_start:
	b startup

    .word 0x454d4f48
    .word 0x57455242

startup:
	mov x4, x30

    // clear .bss
	ldr x0, =__bss_start__
	ldr x1, =__bss_end__
	sub x1, x1, x0
	bl  ClearMem

	mov x0, #0 // argc
	mov x1, #0 // argv

	ldr x3, =main
	blr x3

_sysexit:
	svc 0x7

ClearMem:
	mov  x2, #3     // Round down to nearest word boundary
	add  x1, x1, x2 // Shouldn't be needed
	bics x1, x1, x2	// Clear 2 LSB (and set Z)
	beq ClearMem_end         // Quit if copy size is 0

	mov	x2, #0
ClrLoop:
	str x2, [x0], #4
	subs  x1, x1, #4
	bne   ClrLoop

ClearMem_end:
	ret
