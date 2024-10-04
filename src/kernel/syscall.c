#include "syscall.h"
#include "idt.h"
#include "keyboard.h"
#include "task.h"
#include "terminal.h"
#include "process.h"
#include <stdint.h>
#include "kernel.h"
#include "serial.h"
#include "string.h"
#include "status.h"
#include "file.h"

void register_syscalls()
{
    register_syscall(SYSCALL_EXIT, sys_exit);
    register_syscall(SYSCALL_PRINT, sys_print);
    register_syscall(SYSCALL_GETKEY, sys_getkey);
    register_syscall(SYSCALL_PUTCHAR, sys_putchar);
    register_syscall(SYSCALL_MALLOC, sys_malloc);
    register_syscall(SYSCALL_FREE, sys_free);
    register_syscall(SYSCALL_PROCESS_START, sys_process_start);
    register_syscall(SYSCALL_INVOKE_SYSTEM, sys_invoke_system);
    register_syscall(SYSCALL_GET_PROGRAM_ARGUMENTS, sys_get_program_arguments);
    register_syscall(SYSCALL_OPEN, sys_open);
    register_syscall(SYSCALL_CLOSE, sys_close);
    register_syscall(SYSCALL_READ, sys_read);
}

void *sys_read(struct interrupt_frame *frame)
{
    void *ptr = task_get_stack_item(task_current(), 3);
    unsigned int size = (unsigned int)task_get_stack_item(task_current(), 2);
    unsigned int nmemb = (unsigned int)task_get_stack_item(task_current(), 1);
    int fd = (int)task_get_stack_item(task_current(), 0);

    int res = fread(ptr, size, nmemb, fd);
    return (void *)res;
}

void *sys_close(struct interrupt_frame *frame)
{
    int fd = (int)task_get_stack_item(task_current(), 0);

    int res = fclose(fd);
    return (void *)res;
}

void *sys_open(struct interrupt_frame *frame)
{
    void *file_name = task_get_stack_item(task_current(), 1);
    char name[MAX_PATH_LENGTH];

    copy_string_from_task(task_current(), file_name, name, sizeof(name));

    void *mode = task_get_stack_item(task_current(), 0);
    char mode_str[2];

    copy_string_from_task(task_current(), mode, mode_str, sizeof(mode_str));

    int fd = fopen(name, mode_str);
    return (void *)(int)fd;
}

void *sys_exit(struct interrupt_frame *frame)
{
    struct process *process = task_current()->process;
    process_terminate(process);
    task_next();

    return NULL;
}

void *sys_print(struct interrupt_frame *frame)
{
    void *message = task_get_stack_item(task_current(), 0);
    char buffer[1024];

    copy_string_from_task(task_current(), message, buffer, sizeof(buffer));

    kprintf("%s", buffer);

    return NULL;
}

void *sys_getkey(struct interrupt_frame *frame)
{
    char c = keyboard_pop();
    return (void *)(int)c;
}

void *sys_putchar(struct interrupt_frame *frame)
{
    char c = (char)(int)task_get_stack_item(task_current(), 0);
    terminal_write_char(c, 0x0F, 0x00);
    return NULL;
}

void *sys_malloc(struct interrupt_frame *frame)
{
    size_t size = (size_t)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}

void *sys_free(struct interrupt_frame *frame)
{
    void *ptr = task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr);
    return NULL;
}

void *sys_process_start(struct interrupt_frame *frame)
{
    void *process_file = task_get_stack_item(task_current(), 0);
    char *file_name[MAX_PATH_LENGTH];
    int res = copy_string_from_task(task_current(), process_file, file_name, sizeof(file_name));
    if (res < 0)
    {
        dbgprintf("Failed to copy string from task\n");
        goto out;
    }

    // TODO: Handle paths properly
    char path[MAX_PATH_LENGTH];
    strncpy(path, "0:/", sizeof(path));
    strncpy(path + 3, (const char *)file_name, sizeof(path) - 3);

    struct process *process = NULL;
    res = process_load_switch((const char *)path, &process);
    if (res < 0)
    {
        dbgprintf("Failed to load process %s\n", file_name);
        kprintf("\nFailed to load process %s\n", path);
        goto out;
    }

    task_switch(process->task);
    task_return(&process->task->registers);

out:
    return (void *)res;
}

void *sys_invoke_system(struct interrupt_frame *frame)
{
    struct command_argument *arguments = task_virtual_to_physical_address(
        task_current(),
        task_get_stack_item(task_current(), 0));
    if (!arguments || strlen(arguments->argument) == 0)
    {
        dbgprintf("Invalid arguments\n");
        return ERROR(-EINVARG);
    }

    struct command_argument *root_command_argument = &arguments[0];
    const char *program_name = root_command_argument->argument;

    char path[MAX_PATH_LENGTH];
    strncpy(path, "0:/", sizeof(path));
    strncpy(path + 3, program_name, sizeof(path) - 3);

    struct process *process = NULL;
    int res = process_load_switch(path, &process);
    if (res < 0)
    {
        dbgprintf("Failed to load process %s\n", program_name);
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0)
    {
        return ERROR(res);
    }

    task_switch(process->task);
    task_return(&process->task->registers);

    return NULL;
}

void *sys_get_program_arguments(struct interrupt_frame *frame)
{
    struct process *process = task_current()->process;
    struct process_arguments *arguments = task_virtual_to_physical_address(
        task_current(),
        task_get_stack_item(task_current(), 0));

    process_get_arguments(process, &arguments->argc, &arguments->argv);

    return NULL;
}