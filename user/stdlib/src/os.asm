[BITS 32]

; TODO: Is section .asm really needed?
section .asm

; void os_exit();
global os_exit:function
os_exit:
    push ebp
    mov ebp, esp
    mov eax, 0           ; sys_exit command
    int 0x80
    pop ebp
    ret

; void print(const char* str)
global os_print:function
os_print:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; str
    mov eax, 1           ; sys_print command
    int 0x80
    add esp, 4
    pop ebp
    ret

; int os_getkey()
global os_getkey:function
os_getkey:
    push ebp
    mov ebp, esp
    mov eax, 2           ; sys_getkey command
    int 0x80
    pop ebp
    ret

; void* os_malloc(size_t size)
global os_malloc:function
os_malloc:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; size
    mov eax, 4           ; sys_malloc command
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
    mov eax, 5           ; sys_free command
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
    mov eax, 3           ; sys_putchar command
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
    mov eax, 6           ; sys_process_start command
    int 0x80
    add esp, 4
    pop ebp
    ret

; int os_system(struct command_argument* args);
global os_system:function
os_system:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; args
    mov eax, 7           ; sys_invoke_system command
    int 0x80
    add esp, 4
    pop ebp
    ret
    
; void os_process_get_arguments(struct process_arguments* args);
global os_process_get_arguments:function
os_process_get_arguments:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; args
    mov eax, 8           ; sys_process_get_arguments command
    int 0x80
    add esp, 4
    pop ebp
    ret

; int fopen(const char *path, const char *mode)
global os_open:function
os_open:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; path
    push dword [ebp + 12] ; mode
    mov eax, 9           ; sys_open command
    int 0x80
    add esp, 8
    mov esp, ebp
    pop ebp
    ret

; int os_close(int fd);
global os_close:function
os_close:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; fd
    mov eax, 10          ; sys_close command
    int 0x80
    add esp, 4
    ; mov esp, ebp
    pop ebp
    ret

; int fstat(int fd, struct file_stat *stat)
global os_stat:function
os_stat:
    push ebp
    mov ebp, esp
    push dword [ebp + 8] ; fd
    push dword [ebp + 12] ; stat
    mov eax, 11          ; sys_stat command
    int 0x80
    add esp, 8
    mov esp, ebp
    pop ebp
    ret

; int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd)
global os_read:function
os_read:
    push ebp
    mov ebp, esp
    push dword [ebp + 8]  ; ptr
    push dword [ebp + 12] ; size
    push dword [ebp + 16] ; nmemb
    push dword [ebp + 20] ; fd
    mov eax, 12           ; sys_read command
    int 0x80
    add esp, 16
    mov esp, ebp
    pop ebp
    ret

; void clear_screen();
global os_clear_screen:function
os_clear_screen:
    push ebp
    mov ebp, esp
    mov eax, 13          ; sys_clear_screen command
    int 0x80
    pop ebp
    ret

