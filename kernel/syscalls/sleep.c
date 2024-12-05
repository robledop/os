#include <scheduler.h>
#include <stdint.h>
#include <syscall.h>

void *sys_sleep(struct interrupt_frame *frame)
{
    const int time                               = get_integer_argument(0);
    const uint32_t jiffies                       = scheduler_get_jiffies();
    const uint32_t end                           = jiffies + time;
    scheduler_get_current_process()->state       = SLEEPING;
    scheduler_get_current_process()->sleep_until = end;

    schedule();

    return nullptr;
}
