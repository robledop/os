#include <idt.h>
#include <kernel_heap.h>
#include <process.h>
#include <scheduler.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>

void register_syscalls()
{
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
    register_syscall(SYSCALL_WRITE, sys_write);
    register_syscall(SYSCALL_MKDIR, sys_mkdir);
    register_syscall(SYSCALL_GETDENTS, sys_getdents);
    register_syscall(SYSCALL_GETCWD, sys_getcwd);
    register_syscall(SYSCALL_CHDIR, sys_chdir);
    register_syscall(SYSCALL_WAIT_PID, sys_wait_pid);
    register_syscall(SYSCALL_REBOOT, sys_reboot);
    register_syscall(SYSCALL_SHUTDOWN, sys_shutdown);
    register_syscall(SYSCALL_SLEEP, sys_sleep);
    register_syscall(SYSCALL_YIELD, sys_yield);
    register_syscall(SYSCALL_PS, sys_ps);
    register_syscall(SYSCALL_MEMSTAT, sys_memstat);
}

/// @brief Get the pointer argument from the stack of the current task
/// @param index position of the argument in the stack
void *get_pointer_argument(const int index)
{
    return thread_peek_stack_item(scheduler_get_current_thread(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @param index position of the argument in the stack
int get_integer_argument(const int index)
{
    return (int)thread_peek_stack_item(scheduler_get_current_thread(), index);
}

/// @brief Get the int argument from the stack of the current task
/// @warning BEWARE: The string is allocated on the kernel heap and must be freed by the caller
/// @param index position of the argument in the stack
/// @param max_len maximum length of the string
char *get_string_argument(const int index, const size_t max_len)
{
    const void *ptr = get_pointer_argument(index);
    char *str       = kzalloc(max_len + 1);

    copy_string_from_thread(scheduler_get_current_thread(), ptr, str, max_len);
    return (char *)str;
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
