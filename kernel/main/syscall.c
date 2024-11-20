#include <debug.h>
#include <heap.h>
#include <idt.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <memory.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>
#include <vga_buffer.h>
#include <x86.h>

spinlock_t create_process_lock;
spinlock_t fork_lock;
spinlock_t exec_lock;
spinlock_t wait_lock;

struct command_argument *parse_command(char **args);
static void *get_pointer_argument(const int index);
static int get_integer_argument(const int index);
static char *get_string_argument(const int index, const int max_len);

void register_syscalls()
{
    spinlock_init(&create_process_lock);
    spinlock_init(&fork_lock);
    spinlock_init(&exec_lock);
    spinlock_init(&wait_lock);

    register_syscall(SYSCALL_EXIT, sys_exit);
    register_syscall(SYSCALL_GETKEY, sys_getkey);
    register_syscall(SYSCALL_PUTCHAR, sys_putchar);
    register_syscall(SYSCALL_PRINT, sys_print);
    register_syscall(SYSCALL_MALLOC, sys_malloc);
    register_syscall(SYSCALL_CALLOC, sys_calloc);
    register_syscall(SYSCALL_FREE, sys_free);
    register_syscall(SYSCALL_CREATE_PROCESS, sys_create_process);
    register_syscall(SYSCALL_FORK, sys_fork);
    register_syscall(SYSCALL_EXEC, sys_exec);
    register_syscall(SYSCALL_GET_PID, sys_getpid);
    register_syscall(SYSCALL_GET_PROGRAM_ARGUMENTS, sys_get_program_arguments);
    register_syscall(SYSCALL_OPEN, sys_open);
    register_syscall(SYSCALL_CLOSE, sys_close);
    register_syscall(SYSCALL_STAT, sys_stat);
    register_syscall(SYSCALL_READ, sys_read);
    register_syscall(SYSCALL_OPEN_DIR, sys_open_dir);
    register_syscall(SYSCALL_GET_CURRENT_DIRECTORY, sys_get_current_directory);
    register_syscall(SYSCALL_SET_CURRENT_DIRECTORY, sys_set_current_directory);
    register_syscall(SYSCALL_WAIT_PID, sys_wait_pid);
    register_syscall(SYSCALL_REBOOT, sys_reboot);
    register_syscall(SYSCALL_SHUTDOWN, sys_shutdown);
    register_syscall(SYSCALL_SLEEP, sys_sleep);
    register_syscall(SYSCALL_YIELD, sys_yield);
    register_syscall(SYSCALL_PS, sys_ps);
    register_syscall(SYSCALL_MEMSTAT, sys_memstat);
}

void *sys_memstat(struct interrupt_frame *frame)
{
    kernel_heap_print_stats();

    return nullptr;
}

void *sys_print(struct interrupt_frame *frame)
{
    uint32_t size       = get_integer_argument(0);
    const void *message = get_pointer_argument(1);
    if (!message) {
        warningf("message is null\n");
        return nullptr;
    }

    char buffer[size];

    copy_string_from_thread(scheduler_get_current_thread(), message, buffer, sizeof(buffer));

    printf(buffer);

    return nullptr;
}
void *sys_ps(struct interrupt_frame *frame)
{
    struct process_info *proc_info = nullptr;
    int count                      = 0;
    scheduler_get_processes(&proc_info, &count);
    printf(KBOLD KBLU "\n %-5s%-15s%-12s%-12s%-12s\n" KRESET, "PID", "Name", "Priority", "State", "Exit code");
    for (int i = 0; i < count; i++) {
        constexpr int col               = 15;
        const struct process_info *info = &proc_info[i];

        char state[col + 1];
        switch (info->state) {
        case RUNNING:
            strncpy(state, "RUNNING", col);
            break;
        case ZOMBIE:
            strncpy(state, "ZOMBIE", col);
            break;
        case SLEEPING:
            strncpy(state, "SLEEPING", col);
            break;
        case WAITING:
            strncpy(state, "WAITING", col);
            break;
        default:
            strncpy(state, "UNKNOWN", col);
            break;
        }

        printf(" %-5u%-15s%-12u%-12s%-12u\n", info->pid, info->file_name, info->priority, state, info->exit_code);
    }

    return nullptr;
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
    spin_lock(&fork_lock);

    auto const parent            = scheduler_get_current_process();
    auto const child             = process_clone(parent);
    child->thread->registers.eax = 0;

    spin_unlock(&fork_lock);

    return (void *)(int)child->pid;
}

// TODO: Simplify this
// int exec(const char *path, const char *argv[])
void *sys_exec(struct interrupt_frame *frame)
{
    spin_lock(&exec_lock);

    const void *path_ptr  = thread_peek_stack_item(scheduler_get_current_thread(), 1);
    void *virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 0);

    char **args_ptr = nullptr;

    if (virtual_address) {
        args_ptr = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);
    }

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

    struct process *process = scheduler_get_current_process();
    process_free_allocations(process);
    process_free_program_data(process);
    kfree(process->stack);
    process->stack = nullptr;
    paging_free_directory(process->page_directory);
    thread_free(process->thread);
    process->thread = nullptr;
    process_unmap_memory(process);

    char full_path[MAX_PATH_LENGTH] = {0};
    if (istrncmp(path, "/", 1) != 0) {
        strcat(full_path, "/bin/");
        strcat(full_path, path);
    } else {
        strcat(full_path, path);
    }

    const int res = process_load_data(full_path, process);
    if (res < 0) {
        printf("Result: %d\n", res);
        spin_unlock(&exec_lock);
        return (void *)res;
    }
    void *program_stack_pointer = kzalloc(USER_STACK_SIZE);
    strncpy(process->file_name, full_path, sizeof(process->file_name));
    process->stack        = program_stack_pointer; // Physical address of the stack for the process
    struct thread *thread = thread_create(process);
    if (ISERR(thread)) {
        panic("Failed to create thread");
    }
    process->thread = thread;
    process_map_memory(process);

    process_inject_arguments(process, root_argument);

    scheduler_set_process(process->pid, process);
    scheduler_queue_thread(thread);

    spin_unlock(&exec_lock);

    // schedule();

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

    struct dir_entries *directory = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 1));

    ASSERT(directory, "Invalid directory");

    struct dir_entries *dir_tmp;
    int res = fs_open_dir((const char *)path, &dir_tmp);
    if (res < 0) {
        return (void *)res;
    }

    memcpy(directory, dir_tmp, sizeof(struct dir_entries));
    return (void *)res;
}

void *sys_get_program_arguments(struct interrupt_frame *frame)
{
    const struct process *process = scheduler_get_current_thread()->process;
    void *virtual_address         = thread_peek_stack_item(scheduler_get_current_thread(), 0);
    if (!virtual_address) {
        return nullptr;
    }
    struct process_arguments *arguments =
        thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);

    arguments->argc = process->arguments.argc;
    arguments->argv = process->arguments.argv;

    return nullptr;
}

void *sys_stat(struct interrupt_frame *frame)
{
    const int fd          = get_integer_argument(1);
    void *virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 0);

    struct file_stat *stat = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);

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
    const void *file_name       = get_pointer_argument(1);
    char *name[MAX_PATH_LENGTH] = {nullptr};

    copy_string_from_thread(scheduler_get_current_thread(), file_name, name, sizeof(name));

    const void *mode = get_pointer_argument(0);
    char mode_str[2];

    copy_string_from_thread(scheduler_get_current_thread(), mode, mode_str, sizeof(mode_str));

    const int fd = fopen((const char *)name, mode_str);
    return (void *)(int)fd;
}

[[noreturn]] void *sys_exit(struct interrupt_frame *frame)
{
    auto const process = scheduler_get_current_process();
    auto const parent  = process->parent;

    // The parent is not waiting for you, so you can safely remove yourself from the parent's child list.
    // If the parent is waiting for you, then the waitpid() syscall will take care of everything
    if ((parent && parent->state != WAITING) ||
        (parent && parent->state == WAITING && parent->wait_pid != -1 && parent->wait_pid != process->pid)) {
        process_remove_child(parent, process);
    }

    process_zombify(process);
    if (!process->parent) {
        kfree(process);
    }

    cli();
    schedule();

    panic("Trying to schedule a dead thread");

    // This must not return, otherwise we will get a general protection fault.
    // We will only reach this point if the scheduler tries to run a dead thread.
    // As a last resort, we will enable interrupts, halt the CPU, and wait for a rescheduling.
    sti();
    while (1) {
        hlt();
    }

    __builtin_unreachable();
}

void *sys_getkey(struct interrupt_frame *frame)
{
    const uint8_t c = keyboard_pop();
    if (c == 0) {
        scheduler_get_current_thread()->process->state        = SLEEPING;
        scheduler_get_current_thread()->process->sleep_reason = SLEEP_REASON_KEYBOARD;
    }
    return (void *)(int)c;
}

void *sys_putchar(struct interrupt_frame *frame)
{
    const char c = (char)get_integer_argument(0);
    putchar(c);
    return NULL;
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
    const int pid     = get_integer_argument(1);
    void *virtual_ptr = thread_peek_stack_item(scheduler_get_current_thread(), 0);
    int *status_ptr   = nullptr;
    if (virtual_ptr) {
        status_ptr = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_ptr);
    }
    const int status = process_wait_pid(scheduler_get_current_thread()->process, pid);
    if (status_ptr) {
        *status_ptr = status;
    }

    return nullptr;
}

extern int __cli_count;
void *sys_create_process(struct interrupt_frame *frame)
{
    spin_lock(&create_process_lock);

    void *virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 0);
    struct command_argument *arguments =
        thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);
    if (!arguments || strlen(arguments->argument) == 0) {
        warningf("Invalid arguments\n");
        spin_unlock(&create_process_lock);
        return ERROR(-EINVARG);
    }

    struct command_argument *root_command_argument = &arguments[0];
    const char *program_name                       = root_command_argument->argument;

    char path[MAX_PATH_LENGTH];
    strncpy(path, "/bin/", sizeof(path));
    strncpy(path + strlen("/bin/"), program_name, sizeof(path));

    struct process *process = nullptr;
    int res                 = process_load_enqueue(path, &process);
    if (res < 0) {
        warningf("Failed to load process %s\n", program_name);
        process_free(scheduler_get_current_process(), virtual_address);
        process_command_argument_free(root_command_argument);
        spin_unlock(&create_process_lock);
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0) {
        warningf("Failed to inject arguments for process %s\n", program_name);
        process_free(scheduler_get_current_process(), virtual_address);
        process_command_argument_free(root_command_argument);
        spin_unlock(&create_process_lock);
        return ERROR(res);
    }

    struct process *current_process = scheduler_get_current_thread()->process;
    process->parent                 = current_process;
    process->state                  = RUNNING;
    process->priority               = 1;
    process_add_child(current_process, process);

    process_free(scheduler_get_current_process(), virtual_address);
    process_command_argument_free(root_command_argument);
    spin_unlock(&create_process_lock);

    return (void *)(int)process->pid;
}

struct command_argument *parse_command(char **args)
{
    if (*args == nullptr) {
        return nullptr;
    }

    struct command_argument *head = kmalloc(sizeof(struct command_argument));
    if (head == nullptr) {
        return nullptr;
    }

    strncpy(head->argument, *args, sizeof(head->argument));
    head->next = nullptr;

    struct command_argument *current = head;

    int i = 1;
    while (args[i] != nullptr) {
        struct command_argument *next = kmalloc(sizeof(struct command_argument));
        if (next == nullptr) {
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
