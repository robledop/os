global gdt_flush

section .text

%include "constants.asm"

gdt_flush:
    ; Get the pointer to gdt_ptr from the stack
    mov eax, [esp + 4]
    lgdt [eax]        ; Load the new GDT

    ; Reload the segment registers with the new selectors
    mov ax, KERNEL_DATA_SELECTOR      ; Data segment selector offset (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload the code segment selector and flush the pipeline
    jmp 0x08:flush_label

flush_label:
    ret