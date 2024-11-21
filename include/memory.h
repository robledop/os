#pragma once

#include <stddef.h>
#include <stdint.h>

__attribute__((nonnull)) void *memset(void *ptr, int value, size_t size);
__attribute__((nonnull)) void *memcpy(void *dest, const void *src, size_t n);
__attribute__((nonnull)) void *memsetw(void *dest, uint16_t value, size_t n);
__attribute__((nonnull)) void *memmove(void *dest, const void *src, size_t n);
