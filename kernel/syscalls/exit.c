#include <kernel.h>
#include <kernel_heap.h>
#include <syscall.h>
#include <task.h>
#include <x86.h>

[[noreturn]] void *sys_exit(void)
{
    // auto const process = get_current_task()->process;
    // auto const parent  = process->parent;

    // The parent is not waiting for you, so you can safely remove yourself from the parent's child list.
    // If the parent is waiting for you, then the waitpid() syscall will take care of everything
    // if ((parent && parent->state != WAITING) ||
    //     (parent && parent->state == WAITING && parent->wait_pid != -1 && parent->wait_pid != process->pid)) {
    // }

    // process_zombify(process);
    // if (!process->parent) {
    //     kfree(process);
    // }

    tasks_exit();

    cli();

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
