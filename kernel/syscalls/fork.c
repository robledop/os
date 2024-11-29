#include <process.h>
#include <scheduler.h>
#include <spinlock.h>
#include <syscall.h>

spinlock_t fork_lock = 0;

void *sys_fork(struct interrupt_frame *frame)
{
    spin_lock(&fork_lock);

    auto const parent            = scheduler_get_current_process();
    auto const child             = process_clone(parent);
    child->thread->registers.eax = 0;

    spin_unlock(&fork_lock);

    return (void *)(int)child->pid;
}
