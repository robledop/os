;; restore_cpu_context.asm
;; Restore the CPU context from the provided structure
;
;global restore_cpu_context
;section .text
;
;restore_cpu_context:
;    ; Input: [esp + 4] - pointer to cpu_context structure
;
;    ; Save context pointer on the stack
;    mov eax, [esp + 4]     ; EAX = pointer to cpu_context
;    push eax               ; Save context pointer on the stack
;
;    ; Restore EFLAGS
;    push dword [eax + 36]  ; context->eflags
;    popfd
;
;    ; Restore general-purpose registers
;    mov eax, [eax]         ; context->eax
;    mov ecx, [esp + 4 + 4] ; context->ecx (EAX holds context pointer, stack offset adjusted)
;    mov edx, [esp + 4 + 8] ; context->edx
;    mov ebx, [esp + 4 +12] ; context->ebx
;    mov ebp, [esp + 4 +20] ; context->ebp
;    mov esi, [esp + 4 +24] ; context->esi
;    mov edi, [esp + 4 +28] ; context->edi
;
;    ; Restore ESP if necessary (use caution)
;    ; mov esp, [esp + 4 +16]; context->esp
;
;    ; Retrieve context pointer from the stack
;    pop edx                ; EDX = context pointer
;
;    ; Jump to saved EIP
;    jmp [edx + 32]         ; context->eip


; restore_cpu_context.asm
; Correctly restore the CPU context and ensure stack integrity
; NASM syntax

global restore_cpu_context
section .text

restore_cpu_context:
    ; Input: [esp + 4] - pointer to cpu_context structure

    ; Save the context pointer and adjust the stack
    mov edx, [esp + 4]        ; EDX = pointer to cpu_context

    ; Restore general-purpose registers
    mov eax, [edx]            ; Restore EAX from context->eax
    mov ecx, [edx + 4]        ; Restore ECX from context->ecx
    mov edx, [edx + 8]        ; Restore EDX from context->edx
    mov ebx, [edx +12]        ; Restore EBX from context->ebx
    mov ebp, [edx +20]        ; Restore EBP from context->ebp
    mov esi, [edx +24]        ; Restore ESI from context->esi
    mov edi, [edx +28]        ; Restore EDI from context->edi

    ; Restore EFLAGS
    push dword [edx + 36]     ; Push context->eflags onto the stack
    popfd                     ; Pop into EFLAGS

    ; Replace the return address on the stack with the saved EIP
    mov eax, [edx + 32]       ; EAX = context->eip
    mov [esp], eax            ; Overwrite return address with saved EIP

    ; Return to the saved EIP
    ret
