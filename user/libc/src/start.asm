[BITS 32]

section .asm

global _start
extern c_start
extern exit

_start:
    call c_start
    call exit
    ret