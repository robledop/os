#include <printf.h>
#include <syscall.h>

void *sys_putchar(void)
{
    const char c = (char)get_integer_argument(0);
    putchar(c);
    return nullptr;
}
