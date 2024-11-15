[BITS 32]

section .text

global paging_load_directory
global enable_paging

extern print

paging_load_directory:
   push ebp
    mov ebp, esp
    mov eax, [ebp+8]  ; Get the address of the page directory passed as argument
    mov cr3, eax      ; Load the page directory address into CR3
    pop ebp
    ret

enable_paging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, (1 << 31)  ; Set the paging bit in CR0
    mov cr0, eax
    pop ebp
    
    ret