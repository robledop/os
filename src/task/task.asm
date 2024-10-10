[BITS 32]

section .text

global restore_general_purpose_registers
global task_return
global user_registers

; void task_return(struct register* registers)
task_return:
    mov ebp, esp
    ; push the data segment
    ; push stack address
    ; push flags
    ; push code segment
    ; push ip

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

    ; leave kernel mode
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

    ; push ebp                      ; Preserve EBP
    ; mov ebp, esp                  ; Setup new stack frame

    ; mov ebx, [ebp + 8]            ; EBX = pointer to struct registers

    ; mov edi, [ebx + 0]            ; EDI at offset 0
    ; mov esi, [ebx + 4]            ; ESI at offset 4
    ; mov edx, [ebx + 16]           ; EDX at offset 16
    ; mov ecx, [ebx + 20]           ; ECX at offset 20
    ; mov eax, [ebx + 24]           ; EAX at offset 24
    ; mov ebx, [ebx + 12]           ; EBX at offset 12

    ; ; Restore EBP from the struct registers if necessary
    ; ; Caution: This can corrupt the current stack frame
    ; ; Consider handling EBP restoration outside this function
    ; ; mov ebp, [ebx + 8]            ; EBP at offset 8

    ; add esp, 4
    ; pop ebp                       ; Restore original EBP
    ; ret



; void user_registers()
user_registers:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

