#include "syscall.h"
#include "console.h"
#include "file.h"
#include "idt.h"
#include "kernel.h"
#include "keyboard.h"
#include "process.h"
#include "serial.h"
#include "status.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vga_buffer.h"

void register_syscalls() {
    register_syscall(SYSCALL_EXIT, sys_exit);
    register_syscall(SYSCALL_PRINT, sys_print);
    register_syscall(SYSCALL_GETKEY, sys_getkey);
    register_syscall(SYSCALL_PUTCHAR, sys_putchar);
    register_syscall(SYSCALL_MALLOC, sys_malloc);
    register_syscall(SYSCALL_FREE, sys_free);
    register_syscall(SYSCALL_PUTCHAR_COLOR, sys_putchar_color);
    register_syscall(SYSCALL_CREATE_PROCESS, sys_create_process);
    register_syscall(SYSCALL_GET_PROGRAM_ARGUMENTS, sys_get_program_arguments);
    register_syscall(SYSCALL_OPEN, sys_open);
    register_syscall(SYSCALL_CLOSE, sys_close);
    register_syscall(SYSCALL_STAT, sys_stat);
    register_syscall(SYSCALL_READ, sys_read);
    register_syscall(SYSCALL_CLEAR_SCREEN, sys_clear_screen);
    register_syscall(SYSCALL_OPEN_DIR, sys_open_dir);
    register_syscall(SYSCALL_SET_CURRENT_DIRECTORY, sys_set_current_directory);
    register_syscall(SYSCALL_GET_CURRENT_DIRECTORY, sys_get_current_directory);
}

void *sys_set_current_directory(struct interrupt_frame *frame) {
    void *path_ptr = task_get_stack_item(task_current(), 0);
    char path[MAX_PATH_LENGTH];

    copy_string_from_task(task_current(), path_ptr, path, sizeof(path));
    return (void *)process_set_current_directory(task_current()->process, path);
}

void *sys_get_current_directory(struct interrupt_frame *frame) { return (void *)process_current()->current_directory; }

void *sys_open_dir(struct interrupt_frame *frame) {
    void *path_ptr = task_get_stack_item(task_current(), 0);
    char path[MAX_PATH_LENGTH];

    copy_string_from_task(task_current(), path_ptr, path, sizeof(path));

    struct file_directory *directory =
        task_virtual_to_physical_address(task_current(), task_get_stack_item(task_current(), 1));

    // TODO: Use process_malloc to allocate memory for the directory entries
    return (void *)fs_open_dir((const char *)path, directory);
}

void *sys_get_program_arguments(struct interrupt_frame *frame) {
    struct process *process = task_current()->process;
    struct process_arguments *arguments =
        task_virtual_to_physical_address(task_current(), task_get_stack_item(task_current(), 0));

    arguments->argc = process->arguments.argc;
    arguments->argv = process->arguments.argv;

    return NULL;
}

void *sys_clear_screen(struct interrupt_frame *frame) {
    terminal_clear();

    return NULL;
}

void *sys_stat(struct interrupt_frame *frame) {
    int fd = (int)task_get_stack_item(task_current(), 1);

    struct file_stat *stat = task_virtual_to_physical_address(task_current(), task_get_stack_item(task_current(), 0));

    return (void *)fstat(fd, stat);
}

void *sys_read(struct interrupt_frame *frame) {
    void *task_file_contents = task_virtual_to_physical_address(task_current(), task_get_stack_item(task_current(), 3));

    unsigned int size  = (unsigned int)task_get_stack_item(task_current(), 2);
    unsigned int nmemb = (unsigned int)task_get_stack_item(task_current(), 1);
    int fd             = (int)task_get_stack_item(task_current(), 0);

    int res = fread((void *)task_file_contents, size, nmemb, fd);

    return (void *)res;
}

void *sys_close(struct interrupt_frame *frame) {
    int fd = (int)task_get_stack_item(task_current(), 0);

    int res = fclose(fd);
    return (void *)res;
}

void *sys_open(struct interrupt_frame *frame) {
    void *file_name = task_get_stack_item(task_current(), 1);
    char *name[MAX_PATH_LENGTH];

    copy_string_from_task(task_current(), file_name, name, sizeof(name));

    void *mode = task_get_stack_item(task_current(), 0);
    char mode_str[2];

    copy_string_from_task(task_current(), mode, mode_str, sizeof(mode_str));

    int fd = fopen((const char *)name, mode_str);
    return (void *)(int)fd;
}

void *sys_exit(struct interrupt_frame *frame) {
    struct process *process = task_current()->process;
    process_terminate(process);
    task_next();

    return NULL;
}

void *sys_print(struct interrupt_frame *frame) {
    void *message = task_get_stack_item(task_current(), 0);
    char buffer[2048];

    copy_string_from_task(task_current(), message, buffer, sizeof(buffer));

    kprintf("%s", buffer);

    return NULL;
}

void *sys_getkey(struct interrupt_frame *frame) {
    const uchar c = keyboard_pop();
    return (void *)(int)c;
}

void *sys_putchar(struct interrupt_frame *frame) {
    char c = (char)(int)task_get_stack_item(task_current(), 0);
    terminal_write_char(c, 0x0F, 0x00);
    return NULL;
}

void *sys_putchar_color(struct interrupt_frame *frame) {
    const unsigned char backcolor = (unsigned char)(uint32_t)task_get_stack_item(task_current(), 0);
    const unsigned char forecolor = (unsigned char)(uint32_t)task_get_stack_item(task_current(), 1);
    const char c                  = (char)(int)task_get_stack_item(task_current(), 2);

    terminal_write_char(c, forecolor, backcolor);
    return NULL;
}

void *sys_malloc(struct interrupt_frame *frame) {
    const uintptr_t size = (uintptr_t)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}

void *sys_free(struct interrupt_frame *frame) {
    void *ptr = task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr);
    return NULL;
}

void *sys_create_process(struct interrupt_frame *frame) {
    struct command_argument *arguments =
        task_virtual_to_physical_address(task_current(), task_get_stack_item(task_current(), 0));
    if (!arguments || strlen(arguments->argument) == 0) {
        warningf("Invalid arguments\n");
        return ERROR(-EINVARG);
    }

    struct command_argument *root_command_argument = &arguments[0];
    const char *program_name                       = root_command_argument->argument;

    char path[MAX_PATH_LENGTH];
    strncpy(path, "0:/bin/", sizeof(path));
    strncpy(path + 7, program_name, sizeof(path) - 3);

    struct process *process = nullptr;
    int res                 = process_load_switch(path, &process);
    if (res < 0) {
        warningf("Failed to load process %s\n", program_name);
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0) {
        warningf("Failed to inject arguments for process %s\n", program_name);
        return ERROR(res);
    }

#ifndef MULTITASKING
    task_switch(process->task);
    task_return(&process->task->registers);
#endif

    return (void *)(int)process->pid;
}