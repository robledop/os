#include <config.h>
#include <syscall.h>
#include <task.h>

void *sys_chdir(void)
{
    const void *path_ptr = get_pointer_argument(0);
    char path[MAX_PATH_LENGTH];

    copy_string_from_task(get_current_task(), path_ptr, path, sizeof(path));
    return (void *)process_set_current_directory(get_current_task()->process, path);
}
