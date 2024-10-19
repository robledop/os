#include "memory.h"

void *memset(void *ptr, const int value, const size_t size)
{
    unsigned char *p = ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void *memcpy(void *dest, const void *src, const size_t n)
{
    unsigned char *d       = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Compare two memory blocks
int memcmp(const void *v1, const void *v2, unsigned int n)
{
    const unsigned char *s1 = v1;
    const unsigned char *s2 = v2;
    while (n-- > 0) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++, s2++;
    }

    return 0;
}