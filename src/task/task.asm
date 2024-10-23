[BITS 32]

section .text

%include "config.asm"

global restore_general_purpose_registers
global scheduler_run_task_in_user_mode
global set_user_mode_segments

; void scheduler_run_task_in_user_mode(struct register* registers)
scheduler_run_task_in_user_mode:
    mov ebp, esp

    ; access the struct passed as argument
    mov ebx, [ebp + 4]
    ; push the data/stack stack selector
    push dword [ebx + 44]
    ; push the stack pointer
    push dword [ebx + 40]
    ; push the flags
    pushf
    pop eax
    or eax, 0x200
    push eax

    ; push the code segment
    push dword [ebx + 32]

    ; push the instruction pointer
    push dword [ebx + 28]

    ; setup segment registers
    mov ax, [ebx + 44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword [ebp + 4]
    call restore_general_purpose_registers
    add esp, 4

    ; enter user mode
    iretd

; void restore_general_purpose_registers(struct registers* regs)
restore_general_purpose_registers:
    push ebp

    mov ebp, esp
    mov ebx, [ebp + 8]
    mov edi, [ebx]
    mov esi, [ebx + 4]
    mov ebp, [ebx + 8]
    mov edx, [ebx + 16]
    mov ecx, [ebx + 20]
    mov eax, [ebx + 24]
    mov ebx, [ebx + 12]

    add esp, 4
    ret

; void set_user_mode_registers()
set_user_mode_segments:
    mov ax, USER_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

global acquire_lock
acquire_lock:
.retry:
    mov edi, [esp + 4]
    lock bts dword [edi],0
    jc .retry
    ret

global release_lock
release_lock:
    mov edi, [esp + 4]
    lock btr dword [edi],0
    ret