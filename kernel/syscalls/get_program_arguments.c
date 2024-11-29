#include <scheduler.h>
#include <syscall.h>

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
