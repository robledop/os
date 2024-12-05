#include <scheduler.h>
#include <syscall.h>

// TODO: Implement mmap/munmap and use it instead of this
void *sys_calloc(struct interrupt_frame *frame)
{
    const int size  = get_integer_argument(0);
    const int nmemb = get_integer_argument(1);
    return process_calloc(scheduler_get_current_process(), nmemb, size);
}
