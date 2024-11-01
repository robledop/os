#include <debug.h>
#include <elfloader.h>
#include <file.h>
#include <idt.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>
#include <types.h>
#include <vga_buffer.h>

spinlock_t syscall_lock;

void register_syscalls()
{
    spinlock_init(&syscall_lock);

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
    register_syscall(SYSCALL_SLEEP, sys_sleep);
    register_syscall(SYSCALL_YIELD, sys_yield);
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
    return thread_peek_stack_item(scheduler_get_current_thread(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @param index position of the argument in the stack
static int get_integer_argument(const int index)
{
    return (int)thread_peek_stack_item(scheduler_get_current_thread(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @warning BEWARE: The string is allocated on the kernel heap and must be freed by the caller
/// @param index position of the argument in the stack
/// @param max_len maximum length of the string
static char *get_string_argument(const int index, const int max_len)
{
    const void *ptr = get_pointer_argument(index);
    char *str       = kzalloc(max_len);

    copy_string_from_thread(scheduler_get_current_thread(), ptr, str, (int)sizeof(str));
    return (char *)str;
}

void *sys_yield(struct interrupt_frame *frame)
{
    schedule();

    return nullptr;
}

void *sys_sleep(struct interrupt_frame *frame)
{
    const int time                                       = get_integer_argument(0);
    const uint32_t jiffies                               = scheduler_get_jiffies();
    const uint32_t end                                   = jiffies + time;
    scheduler_get_current_thread()->process->state       = SLEEPING;
    scheduler_get_current_thread()->process->sleep_until = end;

    schedule();

    return nullptr;
}

void *sys_getpid(struct interrupt_frame *frame)
{
    return (void *)(int)scheduler_get_current_thread()->process->pid;
}

void *sys_fork(struct interrupt_frame *frame)
{
    spin_lock(&syscall_lock);

    auto const parent            = scheduler_get_current_process();
    auto const child             = process_clone(parent);
    child->thread->registers.eax = 0;

    spin_unlock(&syscall_lock);

    return (void *)(int)child->pid;
}

// TODO: Simplify this
// int exec(const char *path, const char *argv[])
void *sys_exec(struct interrupt_frame *frame)
{
    spin_lock(&syscall_lock);

    const void *path_ptr = thread_peek_stack_item(scheduler_get_current_thread(), 1);

    char **args_ptr = thread_virtual_to_physical_address(scheduler_get_current_thread(),
                                                         thread_peek_stack_item(scheduler_get_current_thread(), 0));

    char path[MAX_PATH_LENGTH] = {0};
    copy_string_from_thread(scheduler_get_current_thread(), path_ptr, path, sizeof(path));

    char *args[256] = {nullptr};
    if (args_ptr) {
        int argc = 0;
        for (int i = 0; i < 256; i++) {
            if (args_ptr[i] == nullptr) {
                break;
            }
            char *arg = kzalloc(256);
            copy_string_from_thread(scheduler_get_current_thread(), args_ptr[i], arg, 256);
            args[i] = arg;
            argc++;
        }
    }

    struct command_argument *root_argument = kzalloc(sizeof(struct command_argument));
    strncpy(root_argument->argument, path, sizeof(root_argument->argument));
    root_argument->current_directory = kzalloc(MAX_PATH_LENGTH);
    strncpy(root_argument->current_directory,
            scheduler_get_current_thread()->process->current_directory,
            sizeof(root_argument->current_directory));

    struct command_argument *arguments = parse_command(args);
    root_argument->next                = arguments;

    // Free the arguments
    for (int i = 0; i < 256; i++) {
        if (args[i] == nullptr) {
            break;
        }
        kfree(args[i]);
    }

    struct process *process = scheduler_get_current_thread()->process;
    process_free_allocations(process);
    process_free_program_data(process);
    kfree(process->stack);
    process->stack = nullptr;
    paging_free_directory(process->page_directory);
    thread_free(process->thread);
    process->thread = nullptr;
    process_unmap_memory(process);

    char full_path[MAX_PATH_LENGTH] = {0};
    if (istrncmp(path, "0:/", 3) != 0) {
        strcat(full_path, "0:/bin/");
        strcat(full_path, path);
    } else {
        strcat(full_path, path);
    }

    const int res = process_load_data(full_path, process);
    if (res < 0) {
        kprintf("Result: %d\n", res);
        return (void *)res;
    }
    void *program_stack_pointer = kzalloc(USER_STACK_SIZE);
    strncpy(process->file_name, full_path, sizeof(process->file_name));
    process->stack      = program_stack_pointer; // Physical address of the stack for the process
    struct thread *task = thread_create(process);
    process->thread     = task;
    process_map_memory(process);

    process_inject_arguments(process, root_argument);

    scheduler_set_process(process->pid, process);
    scheduler_queue_thread(task);

    spin_unlock(&syscall_lock);

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

    copy_string_from_thread(scheduler_get_current_thread(), path_ptr, path, sizeof(path));
    return (void *)process_set_current_directory(scheduler_get_current_thread()->process, path);
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

    copy_string_from_thread(scheduler_get_current_thread(), path_ptr, path, sizeof(path));

    struct file_directory *directory = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 1));

    ASSERT(directory, "Invalid directory");

    // TODO: Use process_malloc to allocate memory for the directory entries
    return (void *)fs_open_dir((const char *)path, directory);
}

void *sys_get_program_arguments(struct interrupt_frame *frame)
{
    const struct process *process       = scheduler_get_current_thread()->process;
    struct process_arguments *arguments = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 0));

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

    struct file_stat *stat = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 0));

    return (void *)fstat(fd, stat);
}

void *sys_read(struct interrupt_frame *frame)
{
    void *task_file_contents = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 3));

    const unsigned int size  = (unsigned int)thread_peek_stack_item(scheduler_get_current_thread(), 2);
    const unsigned int nmemb = (unsigned int)thread_peek_stack_item(scheduler_get_current_thread(), 1);
    const int fd             = (int)thread_peek_stack_item(scheduler_get_current_thread(), 0);

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

    copy_string_from_thread(scheduler_get_current_thread(), file_name, name, sizeof(name));

    const void *mode = get_pointer_argument(0);
    char mode_str[2];

    copy_string_from_thread(scheduler_get_current_thread(), mode, mode_str, sizeof(mode_str));

    const int fd = fopen((const char *)name, mode_str);
    return (void *)(int)fd;
}

__attribute__((noreturn)) void *sys_exit(struct interrupt_frame *frame)
{
    struct process *process = scheduler_get_current_thread()->process;
    process_zombify(process);
    if (!process->parent) {
        kfree(process);
        process = nullptr;
    }

    schedule();

    panic("Trying to schedule a dead task");

    // This must not return, otherwise we will get a general protection fault.
    // We will only reach this point if the scheduler tries to run a dead thread.
    // As a last resort, we will enable interrupts, halt the CPU, and wait for a rescheduling.
    asm volatile("sti");
    while (1) {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

void *sys_print(struct interrupt_frame *frame)
{
    const void *message = get_pointer_argument(0);
    if (!message) {
        warningf("message is null\n");
        return NULL;
    }

    char buffer[2048];

    copy_string_from_thread(scheduler_get_current_thread(), message, buffer, sizeof(buffer));

    kprintf(buffer);

    return NULL;
}

void *sys_getkey(struct interrupt_frame *frame)
{
    const uchar c = keyboard_pop();
    if (c == 0) {
        scheduler_get_current_thread()->process->state = SLEEPING;
    }
    return (void *)(int)c;
}

void *sys_putchar(struct interrupt_frame *frame)
{
    const char c = (char)get_integer_argument(0);
    terminal_putchar(c, DEFAULT_ATTRIBUTE, -1, -1);
    return NULL;
}

void *sys_putchar_color(struct interrupt_frame *frame)
{
    // scheduler_save_current_thread(frame);
    const uint8_t attribute = (unsigned char)get_integer_argument(0);
    const char c            = (char)get_integer_argument(1);

    terminal_putchar(c, attribute, -1, -1);
    // terminal_write_char(c, forecolor, backcolor);
    return nullptr;
}

void *sys_calloc(struct interrupt_frame *frame)
{
    const int size  = get_integer_argument(0);
    const int nmemb = get_integer_argument(1);
    return process_calloc(scheduler_get_current_thread()->process, nmemb, size);
}

void *sys_malloc(struct interrupt_frame *frame)
{
    const uintptr_t size = (uintptr_t)get_pointer_argument(0);
    return process_malloc(scheduler_get_current_thread()->process, size);
}

void *sys_free(struct interrupt_frame *frame)
{
    void *ptr = get_pointer_argument(0);
    process_free(scheduler_get_current_thread()->process, ptr);
    return NULL;
}

void *sys_wait_pid(struct interrupt_frame *frame)
{
    ENTER_CRITICAL();

    const int pid    = get_integer_argument(1);
    int *status_ptr  = thread_virtual_to_physical_address(scheduler_get_current_thread(),
                                                         thread_peek_stack_item(scheduler_get_current_thread(), 0));
    const int status = process_wait_pid(scheduler_get_current_thread()->process, pid);
    if (status_ptr) {
        *status_ptr = status;
    }

    LEAVE_CRITICAL();

    return nullptr;
}

void *sys_create_process(struct interrupt_frame *frame)
{
    struct command_argument *arguments = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 0));
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
    int res                 = process_load_enqueue(path, &process);
    if (res < 0) {
        warningf("Failed to load process %s\n", program_name);
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0) {
        warningf("Failed to inject arguments for process %s\n", program_name);
        return ERROR(res);
    }

    struct process *current_process = scheduler_get_current_thread()->process;
    process->parent                 = current_process;
    process->state                  = RUNNING;
    process->priority               = 1;
    process_add_child(current_process, process);

    return (void *)(int)process->pid;
}