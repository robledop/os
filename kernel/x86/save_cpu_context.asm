;; save_cpu_context.asm
;; Save the CPU context to the provided structure without disrupting execution flow
;
;global save_cpu_context
;section .text
;
;save_cpu_context:
;    ; Input: [esp + 4] - pointer to cpu_context structure
;
;    ; Save all general-purpose registers and EFLAGS
;    pushad                 ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI (32 bytes)
;    pushfd                 ; Push EFLAGS (4 bytes)
;
;    ; Now, the stack looks like:
;    ; [ESP + 0 ]: EFLAGS
;    ; [ESP + 4 ]: EDI
;    ; [ESP + 8 ]: ESI
;    ; [ESP +12 ]: EBP
;    ; [ESP +16 ]: ESP (original value before pushad)
;    ; [ESP +20 ]: EBX
;    ; [ESP +24 ]: EDX
;    ; [ESP +28 ]: ECX
;    ; [ESP +32 ]: EAX
;    ; [ESP +36 ]: Return Address (EIP saved by the call to save_cpu_context)
;    ; [ESP +40 ]: Pointer to cpu_context (argument to save_cpu_context)
;
;    ; Get pointer to cpu_context structure into EDX (avoid using EBX)
;    mov edx, [esp + 40]    ; EDX = pointer to cpu_context
;
;    ; Save EIP (Return Address)
;    mov eax, [esp + 36]    ; EAX = Return Address
;    mov [edx + 32], eax    ; Save EIP to context->eip
;
;    ; Save EFLAGS
;    mov eax, [esp]         ; EAX = EFLAGS
;    mov [edx + 36], eax    ; Save EFLAGS to context->eflags
;
;    ; Save general-purpose registers from the stack into the context structure
;    mov eax, [esp + 32]    ; Saved EAX
;    mov [edx], eax         ; context->eax
;
;    mov eax, [esp + 28]    ; Saved ECX
;    mov [edx + 4], eax     ; context->ecx
;
;    mov eax, [esp + 24]    ; Saved EDX
;    mov [edx + 8], eax     ; context->edx
;
;    mov eax, [esp + 20]    ; Saved EBX
;    mov [edx + 12], eax    ; context->ebx
;
;    mov eax, [esp + 16]    ; Saved ESP (original ESP before pushad)
;    mov [edx + 16], eax    ; context->esp (optional)
;
;    mov eax, [esp + 12]    ; Saved EBP
;    mov [edx + 20], eax    ; context->ebp
;
;    mov eax, [esp + 8]     ; Saved ESI
;    mov [edx + 24], eax    ; context->esi
;
;    mov eax, [esp + 4]     ; Saved EDI
;    mov [edx + 28], eax    ; context->edi
;
;    ; Clean up the stack
;    add esp, 36            ; Remove EFLAGS and all pushed registers (36 bytes)
;
;    ret                    ; Return to the caller


; save_cpu_context.asm
; Correctly save the CPU context without disrupting the stack or return address
; NASM syntax

global save_cpu_context
section .text

save_cpu_context:
    ; Input: [esp + 4] - pointer to cpu_context structure

    ; Save the context pointer
    mov edx, [esp + 4]        ; EDX = pointer to cpu_context

    ; Save EFLAGS
    pushfd                    ; Push EFLAGS onto the stack
    pop eax                   ; Pop into EAX
    mov [edx + 36], eax       ; Save EFLAGS to context->eflags

    ; Save general-purpose registers
    mov [edx], eax            ; Save EAX to context->eax
    mov eax, ecx
    mov [edx + 4], eax        ; Save ECX to context->ecx
    mov eax, edx
    mov [edx + 8], eax        ; Save EDX to context->edx
    mov eax, ebx
    mov [edx + 12], eax       ; Save EBX to context->ebx
    mov eax, esp
    mov [edx + 16], eax       ; Save ESP to context->esp
    mov eax, ebp
    mov [edx + 20], eax       ; Save EBP to context->ebp
    mov eax, esi
    mov [edx + 24], eax       ; Save ESI to context->esi
    mov eax, edi
    mov [edx + 28], eax       ; Save EDI to context->edi

    ; Save EIP (Return Address)
    mov eax, [esp]            ; EAX = Return Address (EIP)
    mov [edx + 32], eax       ; Save EIP to context->eip

    ; Return without modifying the stack
    ret




