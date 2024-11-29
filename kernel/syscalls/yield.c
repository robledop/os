#include <scheduler.h>
#include <syscall.h>

void *sys_yield(struct interrupt_frame *frame)
{
    schedule();

    return nullptr;
}
