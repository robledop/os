[BITS 32]

section .asm

global _start

_start:
    ; invoke the command 0
    ; push 20
    ; push 30

    ; mov eax, 0  ; command number
    ; int 0x80

    ; add esp, 8

    push message
    mov eax, 1 ; print command
    int 0x80
    add esp, 4

    jmp $

section .data
message db "Hello, World!", 0