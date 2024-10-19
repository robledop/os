#pragma once

#include "types.h"
void kernel_heap_init();
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kzalloc(size_t size);
