#include <kernel.h>
#include <syscall.h>

void *sys_reboot(void)
{
    system_reboot();
    return nullptr;
}
