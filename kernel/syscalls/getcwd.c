#include <syscall.h>
#include <task.h>

void *sys_getcwd(void)
{
    return (void *)get_current_task()->process->current_directory;
}
