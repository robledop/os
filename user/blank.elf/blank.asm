[BITS 32]

section .asm

global _start

_start:
    push ebp
    push message
    push len
    mov eax, 2          ; print syscall
    int 0x80
    mov eax, 0           ; sys_exit syscall
    int 0x80
    add esp, 4
    pop ebp
    ret

section .data
message db 0x0a, "This is from blank.elf", 0
len equ $ - message