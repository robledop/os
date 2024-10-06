section .asm

extern syscall_handler
extern interrupt_handler
extern interrupt_pointer_table

global idt_load
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp + 8] ; idt address
    lidt [ebx]

    pop ebp
    ret

%macro interrupt 1
    global int%1
    int%1:
        pushad
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro

%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep

isr80h_wrapper:
    ; interrupt frame start
    ; already pushed by the processor:
    ; uint32_t ip, cs, flags, sp, ss;

    ; pushes the general purpose registers to the stack
    pushad

    ; push the stack pointer
    push esp

    ; eax will contain the syscall
    push eax
    call syscall_handler
    mov dword[tmp_response], eax
    add esp, 8

    ; pops the general purpose registers from the stack
    popad
    mov eax, [tmp_response]
    iretd

section .data
; stores the response of the system call
tmp_response: dd 0

%macro interrupt_array_entry 1
    dd int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep