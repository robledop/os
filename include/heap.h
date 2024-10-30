#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

// ReSharper disable once CppUnusedIncludeDirective
#include "config.h"
#include "types.h"

#define HEAP_BLOCK_TAKEN 0x01
#define HEAP_BLOCK_FREE 0x00
#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table {
    HEAP_BLOCK_TABLE_ENTRY *entries;
    size_t total;
};

struct heap {
    struct heap_table *table;
    void *start;
};

int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table);
void *heap_malloc(const struct heap *heap, const size_t size);
void *heap_realloc(const struct heap *heap, void *ptr, const size_t size);
void heap_free(const struct heap *heap, void *ptr);
