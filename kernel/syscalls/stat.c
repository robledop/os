#include <scheduler.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>

void *sys_stat(struct interrupt_frame *frame)
{
    const int fd          = get_integer_argument(1);
    void *virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 0);

    struct stat *stat = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);

    return (void *)vfs_stat(fd, stat);
}
