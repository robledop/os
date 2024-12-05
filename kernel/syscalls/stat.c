#include <syscall.h>
#include <task.h>
#include <vfs.h>

void *sys_stat(void)
{
    const int fd          = get_integer_argument(1);
    void *virtual_address = task_peek_stack_item(get_current_task(), 0);

    struct stat *stat = thread_virtual_to_physical_address(get_current_task(), virtual_address);

    return (void *)vfs_stat(fd, stat);
}
