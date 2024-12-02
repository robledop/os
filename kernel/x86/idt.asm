section .text

extern kernel_stack_top
extern syscall_handler
extern interrupt_handler
global interrupt_pointer_table

global idt_load
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

idt_load:
    push ebp
    mov ebp, esp

    mov ebx, [ebp + 8] ; idt address
    lidt [ebx]

    pop ebp
    ret

;struct interrupt_frame {
;    uint32_t edi;
;    uint32_t esi;
;    uint32_t ebp;
;    uint32_t reserved;
;    uint32_t ebx;
;    uint32_t edx;
;    uint32_t ecx;
;    uint32_t eax;
;    uint32_t ip;
;    uint32_t cs;
;    uint32_t flags;
;    uint32_t esp;
;    uint32_t ss;

%macro interrupt 1
    global int%1
    int%1:
        ; List of interrupts that push an error code onto the stack
        %if (%1 = 8) || (%1 = 10) || (%1 = 11) || (%1 = 12) || (%1 = 13) || (%1 = 14) || (%1 = 17) || (%1 = 18) || (%1 = 19)
            ; Interrupt pushes error code
            ; Pop error code into EAX
            ; The interrupt handler (C) will need to fetch the error code from EAX, not the stack
            pop eax
            pushad
        %else
            ; No error code
            ; Leave EAX alone
            pushad
        %endif

        push esp                        ; pass the stack pointer as the second argument. It contains the interrupt frame
        push dword %1                   ; pass the interrupt number as the first argument
        call interrupt_handler          ; void void interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
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

    ; eax will contain the syscall id
    push eax
    call syscall_handler
    mov dword[tmp_response], eax ; saves the response of the system call
    add esp, 8

    ; pops the general purpose registers from the stack
    popad
    mov eax, [tmp_response] ; restores the response of the system call
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