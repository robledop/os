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
;global thread_switch
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
    
    
;extern current_task_TCB
;; Define offsets for the thread_control_block fields
;TCB_ESP   equ 0       ; Offset 0x00
;TCB_CR3   equ 4       ; Offset 0x04
;TCB_ESP0  equ 8       ; Offset 0x08
;
;extern tss_entry
;; Define offsets for the TSS fields
;TSS_ESP0 equ 4       ; Offset of ESP0 within the TSS
;
;;C declaration:
;;   void switch_tasks(thread_control_block *next_thread);
;;
;;WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns
;
;global switch_tasks
;switch_tasks:
;
;    ;Save previous task's state
;
;    ;Notes:
;    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
;    ;  EIP is already saved on the stack by the caller's "CALL" instruction
;    ;  The task isn't able to change CR3 so it doesn't need to be saved
;    ;  Segment registers are constants (while running kernel code) so they don't need to be saved
;
;    push ebx
;    push esi
;    push edi
;    push ebp
;
;    mov edi,[current_task_TCB]    ;edi = address of the previous task's "thread control block"
;    mov [edi+TCB_ESP],esp         ;Save ESP for previous task's kernel stack in the thread's TCB
;
;    ;Load next task's state
;
;    mov esi,[esp+(4+1)*4]         ;esi = address of the next task's "thread control block" (parameter passed on stack)
;    mov [current_task_TCB],esi    ;Current task's TCB is the next task TCB
;
;    mov esp,[esi+TCB_ESP]         ;Load ESP for next task's kernel stack from the thread's TCB
;    mov eax,[esi+TCB_CR3]         ;eax = address of page directory for next task
;    mov ebx,[esi+TCB_ESP0]        ;ebx = address for the top of the next task's kernel stack
;    mov [tss_entry + TSS_ESP0],ebx            ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
;    mov ecx,cr3                   ;ecx = previous task's virtual address space
;
;    cmp eax,ecx                   ;Does the virtual address space need to being changed?
;    je .doneVAS                   ; no, virtual address space is the same, so don't reload it and cause TLB flushes
;    mov cr3,eax                   ; yes, load the next task's virtual address space
;.doneVAS:
;
;    pop ebp
;    pop edi
;    pop esi
;    pop ebx
;
;    ret                           ;Load next task's EIP from its kernel stack
    
    
struc task
    .stack:     resd 1
    .page_dir:  resd 1
    .next_task: resd 1
    .state:     resd 1
    .time_used: resq 1
endstruc

%define TASK_RUNNING 0
%define TASK_READY   1

bits    32
section .text
extern  current_task:data, tasks_ready_tail:data
;extern  _tasks_enqueue_ready:function
extern scheduler_queue_thread:function
global  switch_tasks:function
switch_tasks:
    ;Save previous task's state
    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved
    push ebx
    push esi
    push edi
    push ebp

    mov edi,[current_task]        ;edi = address of the previous task's "thread control block"
    mov [edi+task.stack],esp      ;Save ESP for previous task's kernel stack in the thread's TCB
    cmp dword [edi+task.state],TASK_RUNNING
    jne .state_updated
    mov dword [edi+task.state],TASK_READY ;Set the task's state to ready (from running)
    push edi ; push the task to be enqueued
    call scheduler_queue_thread ; call the enqueue function
    add esp,4 ; fix up the stack
 .state_updated:
    ;Load next task's state
    mov esi,[esp+(4+1)*4]         ;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task],esi        ;Current task's TCB is the next task TCB

    mov esp,[esi+task.stack]      ;Load ESP for next task's kernel stack from the thread's TCB

    ; at this point we are operating under the new task's stack

    mov dword [esi+task.state],TASK_RUNNING ;this task is now running
    mov eax,[esi+task.page_dir]   ;eax = address of page directory for next task
    ;mov ebx,[esi+TCB.ESP0]       ;ebx = address for the top of the next task's kernel stack
    ;mov [TSS.ESP0],ebx           ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov ecx,cr3                   ;ecx = previous task's virtual address space

    cmp eax,ecx                   ;Does the virtual address space need to being changed?
    je .done_virt_addr            ; no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,eax                   ; yes, load the next task's virtual address space
.done_virt_addr:
    pop ebp
    pop edi
    pop esi
    pop ebx
    ret                           ;Load next task's EIP from its kernel stack
