#include <memory.h>

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

void *memsetw(void *dest, const uint16_t value, size_t n)
{
    uint16_t *ptr = dest;
    while (n--) {
        *ptr++ = value;
    }
    return dest;
}

void *memmove(void *dest, const void *src, const size_t n)
{
    unsigned char *d       = dest;
    const unsigned char *s = src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}