section .asm

extern int21h_handler
extern no_interrupt_handler
extern print_line
extern isr80h_handler

global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

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
    pushad
    call int21h_handler

    popad

    iret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

isr80h_wrapper:
    ; interrupt frame start
    ; already pushed by the processor:
    ; uint32_t ip, cs, flags, sp, ss;

    ; pushes the general purpose registers to the stack
    pushad

    ; push the stack pointer
    push esp

    ; eax will contain the command
    push eax
    call isr80h_handler
    mov dword[tmp_response], eax
    add esp, 8

    ; pops the general purpose registers from the stack
    popad
    mov eax, [tmp_response]
    iretd

section .data
; stores the response of the system call
tmp_response: dd 0

; section .data
; message1 db "Interrupts enabled", 0
; message2 db "Interrupts disabled", 0