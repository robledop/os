#include <stdint.h>
#include <syscall.h>
#include <task.h>

void *sys_sleep(void)
{
    const int time = get_integer_argument(0);
    tasks_nano_sleep(time);

    return nullptr;
}
