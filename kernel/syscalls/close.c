#include <spinlock.h>
#include <syscall.h>
#include <vfs.h>
#include <x86.h>

spinlock_t close_lock = 0;

void *sys_close(void)
{
    spin_lock(&close_lock);

    const int fd  = get_integer_argument(0);
    const int res = vfs_close(fd);

    spin_unlock(&close_lock);

    return (void *)res;
}
