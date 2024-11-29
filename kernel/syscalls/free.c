#include <process.h>
#include <scheduler.h>
#include <syscall.h>

// TODO: Implement mmap/munmap and use it instead of this
void *sys_free(struct interrupt_frame *frame)
{
    void *ptr = get_pointer_argument(0);
    process_free(scheduler_get_current_process(), ptr);
    return nullptr;
}
