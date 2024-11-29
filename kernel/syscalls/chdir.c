#include <config.h>
#include <scheduler.h>
#include <syscall.h>
#include <thread.h>

void *sys_chdir(struct interrupt_frame *frame)
{
    const void *path_ptr = get_pointer_argument(0);
    char path[MAX_PATH_LENGTH];

    copy_string_from_thread(scheduler_get_current_thread(), path_ptr, path, sizeof(path));
    return (void *)process_set_current_directory(scheduler_get_current_process(), path);
}
