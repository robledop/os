[BITS 32]

section .asm

global paging_load_directory
global enable_paging

extern print_enabling_paging 
extern print_loading_directory

paging_load_directory:
    call print_loading_directory
    push ebp
    mov ebp, esp
    mov eax, [ebp + 8]
    mov cr3, eax
    pop ebp   
    ret

enable_paging:
    call print_enabling_paging
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    pop ebp
    ret