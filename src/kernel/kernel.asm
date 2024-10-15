; NOT USED BY GRUB
[BITS] 32

  global _start
  global cause_problem
  global kernel_registers

  CODE_SEG equ 0x08
  DATA_SEG equ 0x10

_start:
  ; Set selector registers
  mov ax, DATA_SEG
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  ; Remember that the stack grows downwards
  mov ebp, 0x00600000 ; Set stack pointer
;   mov ebp, 0x00EFFFFF ; Set stack pointer
  mov esp, ebp

  ; https://wiki.osdev.org/A20_Line
  in al, 0x92
  or al, 2
  out 0x92, al

  ; Remap the master PIC
  mov al, 00010001b
  out 0x20, al

  mov al, 0x20
  out 0x21, al

  mov al, 00000001b
  out 0x21, al
  ; finished

  mov esp, stack_top
  extern kernel_main
  call kernel_main

 null_loop:
    hlt
    jmp null_loop

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret

  ; divide by zero
cause_problem:
    mov eax, 0
    div eax


section .bss
    align 16
    stack_bottom:
    ; resb 16384 ; 16 KiB
    resb 1048576 ; 1 MiB
    stack_top:

;   times 512-($ - $$) db 0