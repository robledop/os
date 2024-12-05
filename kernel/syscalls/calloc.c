#include <process.h>
#include <syscall.h>

// TODO: Implement mmap/munmap and use it instead of this
void *sys_calloc(void)
{
    const int size  = get_integer_argument(0);
    const int nmemb = get_integer_argument(1);
    return process_calloc(get_current_task()->process, nmemb, size);
}
