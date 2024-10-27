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


    ; Enable blink
;global enable_blink
;enable_blink:
;    ; Read I/O Address 0x03DA to reset index/data flip-flop
;    mov dx, 0x03DA
;    in al, dx
;    ; Write index 0x30 to 0x03C0 to set register index to 0x30
;    mov dx, 0x03C0
;    mov al, 0x30
;    out dx, al
;    ; Read from 0x03C1 to get register contents
;    inc dx
;    in al, dx
;    ; Set Bit 3 to enable Blink
;    or al, 0x08
;    ; Write to 0x03C0 to update register with changed value
;    dec dx
;    out dx, al
