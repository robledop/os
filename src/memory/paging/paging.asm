[BITS 32]

section .asm

global paging_load_directory
global enable_paging

extern print

paging_load_directory:
    ; push message2
    ; call print
    ; add esp, 4

    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr3, eax
    pop ebp   

    ; push message1
    ; call print
    ; add esp, 4

    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ; push message2
    ; call print
    ; add esp, 4
    ret

 
; section .data
; message1 db "Page directory loaded", 0
; message2 db "Loading page directory", 0