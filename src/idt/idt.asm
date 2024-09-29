section .asm

extern int21h_handler
extern no_interrupt_handler
extern print_line

global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts


enable_interrupts:
    sti
    ; push message1
    ; call print_line
    ; add esp, 4
    ret

disable_interrupts:
    cli
    ; push message2
    ; call print_line
    ; add esp, 4
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp + 8] ; idt address
    lidt [ebx]

    pop ebp
    ret

int21h:
    cli
    pushad
    call int21h_handler

    popad
    sti

    iret

no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret

; section .data
; message1 db "Interrupts enabled", 0
; message2 db "Interrupts disabled", 0