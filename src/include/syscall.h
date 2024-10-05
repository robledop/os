#ifndef COMMAND_H
#define COMMAND_H

enum SysCalls
{
    SYSCALL_EXIT,
    SYSCALL_PRINT,
    SYSCALL_GETKEY,
    SYSCALL_PUTCHAR,
    SYSCALL_MALLOC,
    SYSCALL_FREE,
    SYSCALL_PROCESS_START,
    SYSCALL_INVOKE_SYSTEM,
    SYSCALL_GET_PROGRAM_ARGUMENTS,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_STAT,
    SYSCALL_READ,
    SYSCALL_CLEAR_SCREEN
};

void register_syscalls();

struct interrupt_frame;
void *sys_exit(struct interrupt_frame *frame);
void *sys_print(struct interrupt_frame *frame);
void *sys_getkey(struct interrupt_frame *frame);
void *sys_putchar(struct interrupt_frame *frame);
void *sys_malloc(struct interrupt_frame *frame);
void *sys_free(struct interrupt_frame *frame);
// void *sys_process_start(struct interrupt_frame *frame);
void *sys_invoke_system(struct interrupt_frame *frame);
void *sys_get_program_arguments(struct interrupt_frame *frame);
void *sys_open(struct interrupt_frame *frame);
void *sys_close(struct interrupt_frame *frame);
void *sys_stat(struct interrupt_frame *frame);
void *sys_read(struct interrupt_frame *frame);
void *sys_clear_screen(struct interrupt_frame *frame);

#endif