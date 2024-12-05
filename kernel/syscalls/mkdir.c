#include <kernel_heap.h>
#include <syscall.h>
#include <vfs.h>

void *sys_mkdir(void)
{
    char *path    = get_string_argument(0, MAX_PATH_LENGTH);
    const int res = vfs_mkdir(path);
    kfree(path);
    return (void *)res;
}
