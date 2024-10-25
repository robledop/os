; NOT USED BY GRUB
[BITS 32]

%include "config.asm"

  global _start
  global cause_problem
  global set_kernel_mode_segments

_start:
  ; Set selector registers
  mov ax, KERNEL_DATA_SELECTOR
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  ; Remember that the stack grows downwards
  mov ebp, stack_top
  mov esp, ebp

  ; https://wiki.osdev.org/A20_Line
  in al, 0x92
  or al, 2
  out 0x92, al

  ; Remap the master PIC
;  mov al, 00010001b
;  out 0x20, al
;
;  mov al, 0x20
;  out 0x21, al
;
;  mov al, 00000001b
;  out 0x21, al
  ; finished

  extern kernel_main
  call kernel_main

 null_loop:
    hlt
    jmp null_loop

set_kernel_mode_segments:
    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret


section .bss
    align 16
    stack_bottom:
    resb 1048576 ; 1 MiB
    stack_top:

;   times 512-($ - $$) db 0