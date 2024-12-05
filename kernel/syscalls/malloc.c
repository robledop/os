#include <process.h>
#include <stdint.h>
#include <syscall.h>
#include <task.h>

// TODO: Implement mmap/munmap and use it instead of this
void *sys_malloc(void)
{
    const uintptr_t size = (uintptr_t)get_pointer_argument(0);
    return process_malloc(get_current_task()->process, size);
}
