[BITS 32]

; TODO: Is section .asm really needed?
section .asm

; void print(const char* str)
global os_print:function
os_print:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; str
    mov eax, 1           ; print command
    int 0x80
    add esp, 4
    pop ebp
    ret

; int os_getkey()
global os_getkey:function
os_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2           ; getkey command
    int 0x80
    pop ebp
    ret

; void* os_malloc(size_t size)
global os_malloc:function
os_malloc:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; size
    mov eax, 4           ; malloc command
    int 0x80
    add esp, 4
    pop ebp
    ret

; void os_free(void* ptr)
global os_free:function
os_free:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; ptr
    mov eax, 5           ; free command
    int 0x80
    add esp, 4
    pop ebp
    ret

; void putchar(char c)
global os_putchar:function
os_putchar:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; c
    mov eax, 3           ; putchar command
    int 0x80
    add esp, 4
    pop ebp
    ret

; void os_process_start(const char* file_name)
global os_process_start:function
os_process_start:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; file_name
    mov eax, 6           ; process_start command
    int 0x80
    add esp, 4
    pop ebp
    ret
    