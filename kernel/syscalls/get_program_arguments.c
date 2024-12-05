#include <syscall.h>
#include <task.h>

void *sys_get_program_arguments(void)
{
    const struct process *process = get_current_task()->process;
    void *virtual_address         = task_peek_stack_item(get_current_task(), 0);
    if (!virtual_address) {
        return nullptr;
    }
    struct process_arguments *arguments = thread_virtual_to_physical_address(get_current_task(), virtual_address);

    arguments->argc = process->arguments.argc;
    arguments->argv = process->arguments.argv;

    return nullptr;
}
