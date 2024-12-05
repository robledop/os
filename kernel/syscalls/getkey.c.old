#include <keyboard.h>
#include <scheduler.h>
#include <syscall.h>

void *sys_getkey(struct interrupt_frame *frame)
{
    const uint8_t c = keyboard_pop();
    if (c == 0) {
        scheduler_get_current_process()->state        = SLEEPING;
        scheduler_get_current_process()->sleep_reason = SLEEP_REASON_STDIN;
    }
    return (void *)(int)c;
}
