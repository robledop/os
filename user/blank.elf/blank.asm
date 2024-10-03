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

_getkey_loop:
    call getkey
    push eax
    mov eax, 3 ; putchar command
    int 0x80
    add esp, 4

    jmp _getkey_loop

getkey:
    mov eax, 2 ; getkey command
    int 0x80
    cmp eax, 0x00
    je getkey
    ret

section .data
message db "This is from blank.elf", 0