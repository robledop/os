[BITS 32]

section .text

%include "config.asm"

; struct registers {
;    uint32_t edi;      ebx + 0
;    uint32_t esi;      ebx + 4
;    uint32_t ebp;      ebx + 8
;    uint32_t ebx;      ebx + 12
;    uint32_t edx;      ebx + 16
;    uint32_t ecx;      ebx + 20
;    uint32_t eax;      ebx + 24
;
;    uint32_t ip;       ebx + 28
;    uint32_t cs;       ebx + 32
;    uint32_t flags;    ebx + 36
;    uint32_t esp;      ebx + 40
;    uint32_t ss;       ebx + 44
; };

; void scheduler_switch_thread(struct registers* registers)
global thread_switch
thread_switch:
    mov ebp, esp

    ; First we push ip, cs, flags, esp, ss (in this order) to the stack.
    ; These are the registers that are automatically restored by iretd.

    ; access the struct passed as argument
    mov ebx, [ebp + 4]
    ; push the data/stack stack selector
    push dword [ebx + 44]  ; registers->ss
    ; push the stack pointer
    push dword [ebx + 40]  ; registers->esp

    ; push the flags
    ; We also manipulate the flags to make sure interrupts are enabled
    pushfd
    pop eax
    or eax, (1 << 9)          ; enable interrupts
    push eax

    ; push the code segment
    push dword [ebx + 32]  ; registers->cs

    ; push the instruction pointer
    push dword [ebx + 28]  ; registers->eip

    ; setup segment registers
    mov ax, [ebx + 44] ; registers->ss
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; pass the struct as an argument to restore_general_purpose_registers
    push dword [ebp + 4]

    ; Restores the other registers that are not automatically restored by iretd
    call restore_general_purpose_registers

    add esp, 4

    ; enter user mode
    ; pops eip, cs, flags, esp, ss (in this order) from the stack
    ; and jumps to the new address in cs:eip
    iretd

; void restore_general_purpose_registers(struct registers* regs)
global restore_general_purpose_registers
restore_general_purpose_registers:
    push ebp

    mov ebp, esp
    mov ebx, [ebp + 8]   ; get the struct

    mov edi, [ebx + 0]   ; edi = registers->edi
    mov esi, [ebx + 4]   ; esi = registers->esi
    mov ebp, [ebx + 8]   ; ebp = registers->ebp
    mov edx, [ebx + 16]  ; edx = registers->edx
    mov ecx, [ebx + 20]  ; ecx = registers->ecx
    mov eax, [ebx + 24]  ; eax = registers->eax
    mov ebx, [ebx + 12]  ; ebx = registers->ebx

    add esp, 4
    ret

; void set_user_mode_registers()
global set_user_mode_segments
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