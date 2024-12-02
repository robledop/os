#include <syscall.h>

void *sys_idle(struct interrupt_frame *frame)
{
    asm volatile("sti; hlt");
    return nullptr;
}