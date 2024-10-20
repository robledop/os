global tss_flush

section .text

%include "constants.asm"

tss_flush:
    mov ax, TSS_SELECTOR      ; TSS segment selector (index 5, ring 3)
    ltr ax                    ; Load the Task Register
    ret