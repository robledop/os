[BITS 32]

section .text

global paging_load_directory
global enable_paging

extern print

paging_load_directory:
   push ebp
    mov ebp, esp
    mov eax, [ebp+8]  ; Get the address of the page directory from the stack
    mov cr3, eax      ; Load the page directory address into CR3
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    
    ret

 
; section .data
; message1 db "Page directory loaded", 0
; message2 db "Loading page directory", 0