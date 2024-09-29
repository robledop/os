#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "terminal/terminal.h"
#include "memory/memory.h"
#include "string/string.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

// https://wiki.osdev.org/Memory_Map_(x86)

void kheap_init()
{
    // print("Initializing kernel heap\n");
    int total_table_entries = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)HEAP_TABLE_ADDRESS;
    kernel_heap_table.total = total_table_entries;

    // print("Total table entries: ");
    // print(int_to_string(total_table_entries));
    // print("\n");
    // print("Heap table address: ");
    // print(hex_to_string((uint32_t)kernel_heap_table.entries));
    // print("\n");
    // print("Heap table total entries: ");
    // print(int_to_string(kernel_heap_table.total));
    // print("\n");

    void *end = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTES);
    int res = heap_create(&kernel_heap, (void *)HEAP_ADDRESS, end, &kernel_heap_table);
    if (res < 0)
    {
        // TODO: panic
        print("Failed to create heap\n");
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
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void kfree(void *ptr)
{
    heap_free(&kernel_heap, ptr);
}