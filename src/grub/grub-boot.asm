; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
MBFLAGS  equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS)   ; checksum of above, to prove we are multiboot

section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

section .bss
align 16
stack_bottom:
; resb 16384 ; 16 KiB
resb 1048576 ; 1 MiB
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
    ; Set up the stack
	mov esp, stack_top

    ; Push the multiboot header and the magic number as parameter to be used in kernel_main
    push eax
    push ebx

    ; The PIC is a hardware component that manages interrupt signals from peripheral devices to the CPU.
    ; The x86 architecture has two PICs: master and slave. The master PIC is connected to the CPU and 
    ; the slave PIC is connected to the master PIC.
    ; By default, the PICs are mapped to the interrupt vectors 0x08-0x0F and 0x70-0x77. That means that
    ; the PICs are mapped to the same interrupt vectors as the CPU exceptions. To avoid conflicts, we
    ; need to remap the PICs to different interrupt vectors.

    ; ; Remap the master PIC
    mov al, 00010001b
    out 0x20, al

    mov al, 0x20
    out 0x21, al

    mov al, 00000001b
    out 0x21, al
    ; finished

	extern kernel_main
	call kernel_main

	cli
.hang:	hlt
	jmp .hang
.end: