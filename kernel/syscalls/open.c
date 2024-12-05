#include <config.h>
#include <dirent.h>
#include <spinlock.h>
#include <syscall.h>
#include <task.h>
#include <vfs.h>

spinlock_t open_lock = 0;

void *sys_open(void)
{
    spin_lock(&open_lock);

    const void *file_name       = get_pointer_argument(1);
    char *name[MAX_PATH_LENGTH] = {nullptr};

    copy_string_from_task(get_current_task(), file_name, name, sizeof(name));

    const FILE_MODE mode = get_integer_argument(0);

    const int fd = vfs_open((const char *)name, mode);

    spin_unlock(&open_lock);

    return (void *)(int)fd;
}
