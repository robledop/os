%include "config.asm"

bits    32
section .text

;struct context
;{
;    uint edi;
;    uint esi;
;    uint ebx;
;    uint ebp;
;    uint eip;
;};
    
; from xv6
;void  switch_context(struct context**, struct context*);
global switch_context
switch_context:
    mov eax, [esp + 4]
    mov edx, [esp + 8]

    ; Save old callee-saved registers
    push ebp
    push ebx
    push esi
    push edi

    ; Switch stacks
    mov [eax], esp
    mov esp, edx

    ; Load new callee-saved registers
    pop edi
    pop esi
    pop ebx
    pop ebp
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
