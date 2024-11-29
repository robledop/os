#include <syscall.h>
#include <vfs.h>

void *sys_close(struct interrupt_frame *frame)
{
    const int fd = get_integer_argument(0);

    const int res = vfs_close(fd);
    return (void *)res;
}
