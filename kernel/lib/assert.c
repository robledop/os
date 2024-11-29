#include <assert.h>
#include <stdarg.h>
#include <termcolors.h>
#ifdef __KERNEL__
#include <kernel.h>
#include <printf.h>
#else
#include <stdio.h>
#endif

void assert(const char *snippet, const char *file, int line, const char *message, ...)
{
    printf(KBWHT "\nassert failed %s:%d %s" KWHT, file, line, snippet);

    if (*message) {
        va_list arg;
        va_start(arg, message);
        const char *data = va_arg(arg, char *);
        printf(data, arg);
#ifdef __KERNEL__
        panic(message);
#else
        abort();
#endif
    }
#ifdef __KERNEL__
    panic("Assertion failed");
#else
    abort();
#endif
}
