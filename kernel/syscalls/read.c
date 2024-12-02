#include <scheduler.h>
#include <spinlock.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>

spinlock_t read_lock = 0;

void *sys_read(struct interrupt_frame *frame)
{
    // spin_lock(&read_lock);

    void *task_file_contents = thread_virtual_to_physical_address(
        scheduler_get_current_thread(), thread_peek_stack_item(scheduler_get_current_thread(), 3));

    const unsigned int size  = (unsigned int)thread_peek_stack_item(scheduler_get_current_thread(), 2);
    const unsigned int nmemb = (unsigned int)thread_peek_stack_item(scheduler_get_current_thread(), 1);
    const int fd             = (int)thread_peek_stack_item(scheduler_get_current_thread(), 0);

    const int res = vfs_read((void *)task_file_contents, size, nmemb, fd);

    // spin_lock(&read_lock);

    return (void *)res;
}
