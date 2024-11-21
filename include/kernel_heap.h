#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <stddef.h>

void kernel_heap_init(void);
void *kmalloc(size_t size);
__attribute__((nonnull)) void kfree(void *ptr);
void *kzalloc(size_t size);
__attribute__((nonnull)) void *krealloc(void *ptr, const size_t size);
void kernel_heap_print_stats();
