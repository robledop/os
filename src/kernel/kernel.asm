  [BITS] 32

  global _start
  global cause_problem
  extern kernel_main
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
  mov ebp, 0x00200000
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

  call kernel_main

 null_loop:
    hlt
    jmp null_loop

  ; divide by zero
cause_problem:
    mov eax, 0
    div eax

  times 512-($ - $$) db 0