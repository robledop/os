#include "memory.h"

void *memset(void *ptr, int value, size_t size)
{
    unsigned char *p = ptr;
    for (size_t i = 0; i < size; i++)
    {
        p[i] = (unsigned char)value;
    }
    return ptr;
}