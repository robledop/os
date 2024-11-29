#include <kernel.h>
#include <syscall.h>

void *sys_shutdown(struct interrupt_frame *frame)
{
    system_shutdown();
    return nullptr;
}
