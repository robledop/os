#ifndef STDLIB_MEMORY_H
#define STDLIB_MEMORY_H

#include <stddef.h>

void *memset(void *ptr, int value, size_t size);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *v1, const void *v2, unsigned int n);
#endif