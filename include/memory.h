#pragma once

#include "types.h"

void *memset(void *ptr, int value, size_t size);
void *memcpy(void *dest, const void *src, size_t n);
void *memsetw(void *dest, uint16_t value, size_t n);
void *memmove(void *dest, const void *src, size_t n);
