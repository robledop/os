#include <scheduler.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>

/// @brief Get directory entries
void *sys_getdents(struct interrupt_frame *frame)
{
    const int count              = get_integer_argument(0);
    void *buffer_virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 1);
    const int fd                 = get_integer_argument(2);

    struct dirent *buffer = thread_virtual_to_physical_address(scheduler_get_current_thread(), buffer_virtual_address);

    const int res = vfs_getdents(fd, buffer, count);

    return (void *)res;
}
