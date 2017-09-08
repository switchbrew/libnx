	.section ".crt0","ax"
	.global _start, __service_ptr, __apt_appid, __heap_size, __linear_heap_size, __system_arglist, __system_runflags
#---------------------------------------------------------------------------------
_start:
#---------------------------------------------------------------------------------
	bl startup
startup:
	mov x4, x30
	sub x4, x4, #4 //x4 = _start addr

	// Clear the BSS section
	ldr x0, =__bss_start__
	ldr x1, =__bss_end__
	sub x1, x1, x0
	add x0, x0, x4
	bl  ClearMem

	// System initialization
	//mov x0, x4
	//bl initSystem

	// Set up argc/argv arguments for main()
	/*ldr x0, =__system_argc
	ldr x1, =__system_argv
	ldr x0, [x0]
	ldr x1, [x1]*/
	mov x0, #0
	mov x1, #0

	// Jump to user code
	ldr x3, =main
	add x3, x3, x4
	//ldr x30, =__ctru_exit
	//br  x3
	blr x3

	_sysexit:
	svc 0x7
	b .

#---------------------------------------------------------------------------------
# Clear memory to 0x00 if length != 0
#  x0 = Start Address
#  x1 = Length
#---------------------------------------------------------------------------------
ClearMem:
#---------------------------------------------------------------------------------
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
