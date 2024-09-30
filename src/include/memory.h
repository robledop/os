#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *memset(void *ptr, int value, size_t size);
void *memcpy(void *dest, const void *src, size_t n);

#endif