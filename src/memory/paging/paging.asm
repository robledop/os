[BITS 32]

section .asm

global paging_load_directory
global enable_paging


paging_load_directory:
    push ebp
    mov ebp, esp

    cli
    mov eax, [ebp + 8]
    mov cr3, eax

    ; sti                     ; Enable interrupts
    mov esp, ebp
    pop ebp   

    ret

; enable_paging:
;     push ebp
;     mov ebp, esp
;     mov eax, cr0
;     or eax, 0x80000000
;     mov cr0, eax

;     mov esp, ebp
;     pop ebp
;     ret

enable_paging:
    cli                     ; Disable interrupts
    mov eax, cr0
    or eax, 0x80000000      ; Set PG bit to enable paging
    mov cr0, eax
    mov eax, cr3            ; Reload CR3 to flush TLB
    mov cr3, eax
    ; sti                     ; Enable interrupts
    ret

 
; section .data
; message1 db "Page directory loaded", 0
; message2 db "Loading page directory", 0