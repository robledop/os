#include <idt.h>
#include <process.h>
#include <spinlock.h>
#include <syscall.h>

spinlock_t fork_lock = 0;

void *sys_fork(void)
{
    spin_lock(&fork_lock);

    auto const parent              = get_current_task()->process;
    auto const child               = process_clone(parent);
    child->thread->trap_frame->eax = 0;

    spin_unlock(&fork_lock);

    return (void *)(int)child->pid;
}
