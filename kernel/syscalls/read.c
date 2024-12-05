#include <syscall.h>
#include <task.h>
#include <vfs.h>

void *sys_read(void)
{
    void *task_file_contents =
        thread_virtual_to_physical_address(get_current_task(), task_peek_stack_item(get_current_task(), 3));

    const unsigned int size  = (unsigned int)task_peek_stack_item(get_current_task(), 2);
    const unsigned int nmemb = (unsigned int)task_peek_stack_item(get_current_task(), 1);
    const int fd             = (int)task_peek_stack_item(get_current_task(), 0);

    const int res = vfs_read((void *)task_file_contents, size, nmemb, fd);

    return (void *)res;
}
