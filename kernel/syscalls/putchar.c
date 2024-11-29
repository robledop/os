#include <printf.h>
#include <syscall.h>

void *sys_putchar(struct interrupt_frame *frame)
{
    const char c = (char)get_integer_argument(0);
    putchar(c);
    return nullptr;
}
