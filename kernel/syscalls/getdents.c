#include <syscall.h>
#include <task.h>
#include <vfs.h>

/// @brief Get directory entries
void *sys_getdents(void)
{
    const int count              = get_integer_argument(0);
    void *buffer_virtual_address = task_peek_stack_item(get_current_task(), 1);
    const int fd                 = get_integer_argument(2);

    struct dirent *buffer = thread_virtual_to_physical_address(get_current_task(), buffer_virtual_address);

    const int res = vfs_getdents(fd, buffer, count);

    return (void *)res;
}
