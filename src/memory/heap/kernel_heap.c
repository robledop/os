#include "kernel_heap.h"
#include "heap.h"
#include "config.h"
#include "memory.h"
#include "kernel.h"
#include "string.h"
#include "serial.h"
#include "assert.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

// https://wiki.osdev.org/Memory_Map_(x86)

void kernel_heap_init()
{
    int total_table_entries = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)HEAP_TABLE_ADDRESS;
    kernel_heap_table.total = total_table_entries;

    void *end = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void *)HEAP_ADDRESS, end, &kernel_heap_table);
    if (res < 0)
    {
        warningf("Failed to create heap\n");
        panic("Failed to create heap\n");
    }
}

void *kmalloc(size_t size)
{
    return heap_malloc(&kernel_heap, size);
}

void *kzalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (!ptr)
    {
        warningf("Failed to allocate memory\n");
        ASSERT(false, "Failed to allocate memory");
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void kfree(void *ptr)
{
    heap_free(&kernel_heap, ptr);
}