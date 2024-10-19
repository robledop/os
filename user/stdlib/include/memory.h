#pragma once

#include "types.h"

void *memset(void *ptr, int value, size_t size);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *v1, const void *v2, unsigned int n);