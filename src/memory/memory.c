#include "memory.h"
#include "serial.h"
#include "terminal.h"

void *memset(void *ptr, int value, size_t size)
{
    unsigned char *p = ptr;
    for (size_t i = 0; i < size; i++)
    {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}