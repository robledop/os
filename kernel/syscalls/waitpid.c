#include <scheduler.h>
#include <spinlock.h>
#include <syscall.h>
#include <thread.h>

spinlock_t wait_lock = 0;

void *sys_wait_pid(struct interrupt_frame *frame)
{
    const int pid     = get_integer_argument(1);
    void *virtual_ptr = thread_peek_stack_item(scheduler_get_current_thread(), 0);
    int *status_ptr   = nullptr;
    if (virtual_ptr) {
        status_ptr = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_ptr);
    }
    const int status = process_wait_pid(scheduler_get_current_process(), pid);
    if (status_ptr) {
        *status_ptr = status;
    }

    return nullptr;
}
