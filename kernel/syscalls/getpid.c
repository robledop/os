#include <scheduler.h>
#include <syscall.h>

void *sys_getpid(struct interrupt_frame *frame)
{
    return (void *)(int)scheduler_get_current_process()->pid;
}
