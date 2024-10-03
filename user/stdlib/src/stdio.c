#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include <stdarg.h>
#include "string.h"

int putchar(int c)
{
    os_putchar((char)c);
    return 0;
}

int printf(const char *format, ...)
{
    va_list args;
    const char *p;
    char *sval;
    int ival;

    va_start(args, format);
    for (p = format; *p; p++)
    {
        if (*p != '%')
        {
            putchar(*p);
            continue;
        }

        switch (*++p)
        {
        case 'i':
        case 'd':
            ival = va_arg(args, int);
            os_print(itoa(ival));
            break;
        case 's':
            sval = va_arg(args, char *);
            os_print(sval);
            break;
        default:
            putchar(*p);
            break;
        }
    }

    va_end(args);

    return 0;
}