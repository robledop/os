#include <syscall.h>
#include <task.h>

void *sys_getpid(void)
{
    return (void *)(int)get_current_task()->process->pid;
}
