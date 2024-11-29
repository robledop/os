#include <kernel.h>
#include <scheduler.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>

spinlock_t create_process_lock = 0;

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
