[BITS 32]

section .text

%include "config.asm"

global set_kernel_mode_segments
set_kernel_mode_segments:
    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret