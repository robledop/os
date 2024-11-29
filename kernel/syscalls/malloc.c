#include <process.h>
#include <scheduler.h>
#include <stdint.h>
#include <syscall.h>

// TODO: Implement mmap/munmap and use it instead of this
void *sys_malloc(struct interrupt_frame *frame)
{
    const uintptr_t size = (uintptr_t)get_pointer_argument(0);
    return process_malloc(scheduler_get_current_process(), size);
}
