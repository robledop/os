#include <kernel.h>
#include <syscall.h>

void *sys_shutdown(void)
{
    system_shutdown();
    return nullptr;
}
