section .asm

global tss_load

tss_load:
    push ebp
    mov ebp, esp
    mov ax, [ebp + 8] ; Load the TSS address
    ltr ax
    pop ebp
    ret