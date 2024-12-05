#include <kernel.h>
#include <syscall.h>

void *sys_reboot(struct interrupt_frame *frame)
{
    system_reboot();
    return nullptr;
}
