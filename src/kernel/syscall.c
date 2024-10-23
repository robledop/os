#include "syscall.h"

#include <assert.h>
#include <elfloader.h>
#include <kernel_heap.h>
#include <scheduler.h>
#include <spinlock.h>

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

spinlock_t syscall_lock = 0;

void register_syscalls()
{
    register_syscall(SYSCALL_EXIT, sys_exit);
    register_syscall(SYSCALL_PRINT, sys_print);
    register_syscall(SYSCALL_GETKEY, sys_getkey);
    register_syscall(SYSCALL_PUTCHAR, sys_putchar);
    register_syscall(SYSCALL_MALLOC, sys_malloc);
    register_syscall(SYSCALL_CALLOC, sys_calloc);
    register_syscall(SYSCALL_FREE, sys_free);
    register_syscall(SYSCALL_PUTCHAR_COLOR, sys_putchar_color);
    register_syscall(SYSCALL_CREATE_PROCESS, sys_create_process);
    register_syscall(SYSCALL_FORK, sys_fork);
    register_syscall(SYSCALL_EXEC, sys_exec);
    register_syscall(SYSCALL_GET_PID, sys_getpid);
    register_syscall(SYSCALL_GET_PROGRAM_ARGUMENTS, sys_get_program_arguments);
    register_syscall(SYSCALL_OPEN, sys_open);
    register_syscall(SYSCALL_CLOSE, sys_close);
    register_syscall(SYSCALL_STAT, sys_stat);
    register_syscall(SYSCALL_READ, sys_read);
    register_syscall(SYSCALL_CLEAR_SCREEN, sys_clear_screen);
    register_syscall(SYSCALL_OPEN_DIR, sys_open_dir);
    register_syscall(SYSCALL_GET_CURRENT_DIRECTORY, sys_get_current_directory);
    register_syscall(SYSCALL_SET_CURRENT_DIRECTORY, sys_set_current_directory);
    register_syscall(SYSCALL_WAIT_PID, sys_wait_pid);
    register_syscall(SYSCALL_REBOOT, sys_reboot);
    register_syscall(SYSCALL_SHUTDOWN, sys_shutdown);
}

struct command_argument *parse_command(char **args)
{
    if (args[0] == NULL) {
        return nullptr;
    }

    struct command_argument *head = kmalloc(sizeof(struct command_argument));
    if (head == NULL) {
        return nullptr;
    }

    strncpy(head->argument, args[0], sizeof(head->argument));
    head->next = nullptr;

    struct command_argument *current = head;

    int i = 1;
    while (args[i] != NULL) {
        struct command_argument *next = kmalloc(sizeof(struct command_argument));
        if (next == NULL) {
            break;
        }

        strncpy(next->argument, args[i], sizeof(next->argument));
        next->next    = nullptr;
        current->next = next;
        current       = next;
        i++;
    }

    return head;
}


/// @brief Get the pointer argument from the stack of the current task
/// @param index position of the argument in the stack
static void *get_pointer_argument(const int index)
{
    return task_peek_stack_item(scheduler_get_current_task(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @param index position of the argument in the stack
static int get_integer_argument(const int index)
{
    return (int)task_peek_stack_item(scheduler_get_current_task(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @warning BEWARE: The string is allocated on the kernel heap and must be freed by the caller
/// @param index position of the argument in the stack
/// @param max_len maximum length of the string
static char *get_string_argument(const int index, const int max_len)
{
    const void *ptr = get_pointer_argument(index);
    char *str       = kzalloc(max_len);

    copy_string_from_task(scheduler_get_current_task(), ptr, str, (int)sizeof(str));
    return (char *)str;
}

void *sys_getpid(struct interrupt_frame *frame)
{
    return (void *)(int)scheduler_get_current_task()->process->pid;
}

void *sys_fork(struct interrupt_frame *frame)
{
    ENTER_CRITICAL();

    auto const parent          = scheduler_get_current_process();
    auto const child           = process_clone(parent);
    child->task->registers.eax = 0;

    LEAVE_CRITICAL();

    return (void *)(int)child->pid;
}

// int exec(const char *path, const char *argv[])
void *sys_exec(struct interrupt_frame *frame)
{
    ENTER_CRITICAL();

    const void *path_ptr = task_peek_stack_item(scheduler_get_current_task(), 1);

    char **args_ptr = task_virtual_to_physical_address(scheduler_get_current_task(),
                                                       task_peek_stack_item(scheduler_get_current_task(), 0));

    char path[MAX_PATH_LENGTH] = {0};
    copy_string_from_task(scheduler_get_current_task(), path_ptr, path, sizeof(path));

    char *args[256] = {nullptr};
    if (args_ptr) {
        int argc = 0;
        for (int i = 0; i < 256; i++) {
            if (args_ptr[i] == nullptr) {
                break;
            }
            char *arg = kzalloc(256);
            copy_string_from_task(scheduler_get_current_task(), args_ptr[i], arg, 256);
            args[i] = arg;
            argc++;
        }
    }

    struct command_argument *root_argument = kzalloc(sizeof(struct command_argument));
    strncpy(root_argument->argument, path, sizeof(root_argument->argument));
    root_argument->current_directory = kzalloc(MAX_PATH_LENGTH);
    strncpy(root_argument->current_directory,
            scheduler_get_current_task()->process->current_directory,
            sizeof(root_argument->current_directory));

    struct command_argument *arguments = parse_command(args);
    root_argument->next                = arguments;

    struct process *process = scheduler_get_current_task()->process;
    process_unmap_memory(process);
    process_free_allocations(process);
    process_free_program_data(process);
    kfree(process->stack);
    task_free(process->task);

    char full_path[MAX_PATH_LENGTH] = {0};
    if (istrncmp(path, "0:/", 3) != 0) {
        strcat(full_path, "0:/bin/");
        strcat(full_path, path);
    } else {
        strcat(full_path, path);
    }

    int res = process_load_data(full_path, process);
    if (res < 0) {
        kprintf("Result: %d\n", res);
        return (void *)res;
    }
    void *program_stack_pointer = kzalloc(USER_STACK_SIZE);
    strncpy(process->file_name, full_path, sizeof(process->file_name));
    process->stack    = program_stack_pointer; // Physical address of the stack for the process
    struct task *task = task_create(process);
    process->task     = task;
    process_map_memory(process);

    process_inject_arguments(process, root_argument);

    scheduler_set_process(process->pid, process);
    scheduler_queue_task(task);

    LEAVE_CRITICAL();

    schedule();

    return (void *)nullptr;
}


void *sys_reboot(struct interrupt_frame *frame)
{
    system_reboot();
    return nullptr;
}

void *sys_shutdown(struct interrupt_frame *frame)
{
    system_shutdown();
    return nullptr;
}


void *sys_set_current_directory(struct interrupt_frame *frame)
{
    const void *path_ptr = get_pointer_argument(0);
    char path[MAX_PATH_LENGTH];

    copy_string_from_task(scheduler_get_current_task(), path_ptr, path, sizeof(path));
    return (void *)process_set_current_directory(scheduler_get_current_task()->process, path);
}

void *sys_get_current_directory(struct interrupt_frame *frame)
{
    return (void *)scheduler_get_current_process()->current_directory;
}

void *sys_open_dir(struct interrupt_frame *frame)
{
    const void *path_ptr = get_pointer_argument(0);
    ASSERT(path_ptr, "Invalid path");
    char path[MAX_PATH_LENGTH];

    copy_string_from_task(scheduler_get_current_task(), path_ptr, path, sizeof(path));

    struct file_directory *directory = task_virtual_to_physical_address(
        scheduler_get_current_task(), task_peek_stack_item(scheduler_get_current_task(), 1));

    ASSERT(directory, "Invalid directory");

    // TODO: Use process_malloc to allocate memory for the directory entries
    return (void *)fs_open_dir((const char *)path, directory);
}

void *sys_get_program_arguments(struct interrupt_frame *frame)
{
    const struct process *process       = scheduler_get_current_task()->process;
    struct process_arguments *arguments = task_virtual_to_physical_address(
        scheduler_get_current_task(), task_peek_stack_item(scheduler_get_current_task(), 0));

    arguments->argc = process->arguments.argc;
    arguments->argv = process->arguments.argv;

    return NULL;
}

void *sys_clear_screen(struct interrupt_frame *frame)
{
    terminal_clear();

    return NULL;
}

void *sys_stat(struct interrupt_frame *frame)
{
    const int fd = get_integer_argument(1);

    struct file_stat *stat = task_virtual_to_physical_address(scheduler_get_current_task(),
                                                              task_peek_stack_item(scheduler_get_current_task(), 0));

    return (void *)fstat(fd, stat);
}

void *sys_read(struct interrupt_frame *frame)
{
    void *task_file_contents = task_virtual_to_physical_address(scheduler_get_current_task(),
                                                                task_peek_stack_item(scheduler_get_current_task(), 3));

    const unsigned int size  = (unsigned int)task_peek_stack_item(scheduler_get_current_task(), 2);
    const unsigned int nmemb = (unsigned int)task_peek_stack_item(scheduler_get_current_task(), 1);
    const int fd             = (int)task_peek_stack_item(scheduler_get_current_task(), 0);

    const int res = fread((void *)task_file_contents, size, nmemb, fd);

    return (void *)res;
}

void *sys_close(struct interrupt_frame *frame)
{
    const int fd = get_integer_argument(0);

    const int res = fclose(fd);
    return (void *)res;
}

void *sys_open(struct interrupt_frame *frame)
{
    const void *file_name = get_pointer_argument(1);
    char *name[MAX_PATH_LENGTH];

    copy_string_from_task(scheduler_get_current_task(), file_name, name, sizeof(name));

    const void *mode = get_pointer_argument(0);
    char mode_str[2];

    copy_string_from_task(scheduler_get_current_task(), mode, mode_str, sizeof(mode_str));

    const int fd = fopen((const char *)name, mode_str);
    return (void *)(int)fd;
}

__attribute__((noreturn)) void *sys_exit(struct interrupt_frame *frame)
{
    ENTER_CRITICAL();

    struct process *process = scheduler_get_current_task()->process;
    process_terminate(process);

    LEAVE_CRITICAL();
    schedule();

    // This must not return, otherwise we will get a general protection fault
    // We will only reach this point if the scheduler tries to run a dead task
    // As a last resort, we will enable interrupts, halt the CPU, and wait for a rescheduling
    asm volatile("sti");
    while (1) {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

void *sys_print(struct interrupt_frame *frame)
{
    const void *message = get_pointer_argument(0);
    char buffer[2048];

    copy_string_from_task(scheduler_get_current_task(), message, buffer, sizeof(buffer));

    kprintf("%s", buffer);

    return NULL;
}

void *sys_getkey(struct interrupt_frame *frame)
{
    const uchar c = keyboard_pop();
    return (void *)(int)c;
}

void *sys_putchar(struct interrupt_frame *frame)
{
    const char c = (char)get_integer_argument(0);
    terminal_write_char(c, 0x0F, 0x00);
    return NULL;
}

void *sys_putchar_color(struct interrupt_frame *frame)
{
    const unsigned char backcolor = (unsigned char)get_integer_argument(0);
    const unsigned char forecolor = (unsigned char)get_integer_argument(1);
    const char c                  = (char)get_integer_argument(2);

    terminal_write_char(c, forecolor, backcolor);
    return nullptr;
}

void *sys_calloc(struct interrupt_frame *frame)
{
    const int size  = get_integer_argument(0);
    const int nmemb = get_integer_argument(1);
    return process_calloc(scheduler_get_current_task()->process, nmemb, size);
}

void *sys_malloc(struct interrupt_frame *frame)
{
    const uintptr_t size = (uintptr_t)get_pointer_argument(0);
    return process_malloc(scheduler_get_current_task()->process, size);
}

void *sys_free(struct interrupt_frame *frame)
{
    void *ptr = get_pointer_argument(0);
    process_free(scheduler_get_current_task()->process, ptr);
    return NULL;
}

void *sys_wait_pid(struct interrupt_frame *frame)
{
    ENTER_CRITICAL();

    const int pid    = get_integer_argument(1);
    int *status_ptr  = task_virtual_to_physical_address(scheduler_get_current_task(),
                                                       task_peek_stack_item(scheduler_get_current_task(), 0));
    const int status = process_wait_pid(scheduler_get_current_task()->process, pid);
    if (status_ptr) {
        *status_ptr = status;
    }

    LEAVE_CRITICAL();

    return nullptr;
}

void *sys_create_process(struct interrupt_frame *frame)
{
    struct command_argument *arguments = task_virtual_to_physical_address(
        scheduler_get_current_task(), task_peek_stack_item(scheduler_get_current_task(), 0));
    if (!arguments || strlen(arguments->argument) == 0) {
        warningf("Invalid arguments\n");
        return ERROR(-EINVARG);
    }

    const struct command_argument *root_command_argument = &arguments[0];
    const char *program_name                             = root_command_argument->argument;

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

    struct process *current_process = scheduler_get_current_task()->process;
    process->parent                 = current_process;
    process->state                  = RUNNING;
    process->priority               = 1;
    process_add_child(current_process, process);


    // scheduler_switch_task(process->task);
    // scheduler_run_task_in_user_mode(&process->task->registers);

    return (void *)(int)process->pid;
}