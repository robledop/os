section .text

%include "config.asm"

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
;    uint32_t reserved; // useless & ignored
;    uint32_t ebx;
;    uint32_t edx;
;    uint32_t ecx;
;    uint32_t eax;
;
;    uint16_t gs;
;    uint16_t padding1;
;    uint16_t fs;
;    uint16_t padding2;
;    uint16_t es;
;    uint16_t padding3;
;    uint16_t ds;
;    uint16_t padding4;
;    uint32_t interrupt_number;
;
;    uint32_t error_code;
;    uint32_t eip;
;    uint16_t cs;
;    uint16_t padding5;
;    uint32_t eflags;
;
;    uint32_t esp;
;    uint16_t ss;
;    uint16_t padding6;
;} __attribute__((packed));



%macro interrupt 1
    global int%1
    int%1:
        ; List of interrupts that push an error code onto the stack
        %if (%1 = 8) || (%1 = 10) || (%1 = 11) || (%1 = 12) || (%1 = 13) || (%1 = 14) || (%1 = 17) || (%1 = 18) || (%1 = 19)
            ; Interrupt pushes error code
        %else
            ; No error code
            push dword 0
        %endif

        push dword %1                   ; push the interrupt number
        push ds
        push es
        push fs
        push gs
        pushad
        
        ; Setup data segment
        mov ax, KERNEL_DATA_SELECTOR
        mov ds, ax
        mov es, ax
        
        push esp                        ; pass the stack pointer as the argument. It contains the interrupt frame we just built
        call interrupt_handler          ; void void interrupt_handler(struct interrupt_frame *frame)
        add esp, 4
        
        jmp trap_return
        
%endmacro

; context switch if the interrupt is a system call
global trap_return:function
trap_return:
    popad
    pop gs
    pop fs
    pop es
    pop ds
    
    add esp, 8                     ; remove the error code and the interrupt number from the stack
    iret

%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep

isr80h_wrapper:
    ; interrupt frame start
    ; already pushed by the processor:
    ; uint32_t ip, cs, flags, sp, ss;
    
    ; No error code
    push dword 0
    push 80                   ; push the interrupt number
    push ds
    push es
    push fs
    push gs

    ; pushes the general purpose registers to the stack
    pushad
    
    ; Setup data segment
    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax

    ; push the stack pointer
    push esp
    call syscall_handler
    add esp, 4
    
    jmp trap_return

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