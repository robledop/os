#include <scheduler.h>
#include <syscall.h>

void *sys_getcwd(struct interrupt_frame *frame)
{
    return (void *)scheduler_get_current_process()->current_directory;
}
