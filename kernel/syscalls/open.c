#include <config.h>
#include <dirent.h>
#include <scheduler.h>
#include <spinlock.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>

spinlock_t open_lock = 0;

void *sys_open(struct interrupt_frame *frame)
{
    spin_lock(&open_lock);

    const void *file_name       = get_pointer_argument(1);
    char *name[MAX_PATH_LENGTH] = {nullptr};

    copy_string_from_thread(scheduler_get_current_thread(), file_name, name, sizeof(name));

    const FILE_MODE mode = get_integer_argument(0);

    const int fd = vfs_open((const char *)name, mode);

    spin_unlock(&open_lock);

    return (void *)(int)fd;
}
