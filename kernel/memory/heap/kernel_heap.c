#include <config.h>
// #include <debug.h>
#include <assert.h>
#include <heap.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memory.h>
#include <printf.h>
#include <serial.h>

struct heap kernel_heap;
struct heap_table kernel_heap_table;
uint32_t allocations = 0;
uint32_t frees       = 0;

// https://wiki.osdev.org/Memory_Map_(x86)

void kernel_heap_init()
{
    constexpr int total_table_entries = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;
    kernel_heap_table.entries         = (HEAP_BLOCK_TABLE_ENTRY *)HEAP_TABLE_ADDRESS;
    kernel_heap_table.total           = total_table_entries;

    auto const end = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTES);
    const int res  = heap_create(&kernel_heap, (void *)HEAP_ADDRESS, end, &kernel_heap_table);
    if (res < 0) {
        panic("Failed to create heap\n");
    }
}

void *kmalloc(const size_t size)
{
    allocations++;
    void *result = heap_malloc(&kernel_heap, size);
    return result;
}

void *kzalloc(const size_t size)
{
    void *ptr = kmalloc(size);
    if (!ptr) {
        panic("Failed to allocate memory\n");
        return NULL;
    }
    memset(ptr, 0x00, size);
    return ptr;
}

void *krealloc(void *ptr, const size_t size)
{
    return heap_realloc(&kernel_heap, ptr, size);
}

void kfree(void *ptr)
{
    frees++;
    heap_free(&kernel_heap, ptr);
}

void kernel_heap_print_stats()
{
    const uint32_t free_blocks = heap_count_free_blocks(&kernel_heap);
    const uint32_t used_blocks = kernel_heap_table.total - free_blocks;
    const uint32_t free_bytes  = free_blocks * HEAP_BLOCK_SIZE;
    const uint32_t used_bytes  = used_blocks * HEAP_BLOCK_SIZE;
    printf("\n %-12s %lu\n", "malloc():", allocations);
    printf(" %-12s %lu\n", "free():", frees);
    printf(" %-12s %lu\n", "Free blocks:", free_blocks);
    printf(" %-12s %.1f MiB (%lu bytes)\n", "Memory used:", (double)used_bytes / 1024 / 1024, used_bytes);
    printf(" %-12s %.1f MiB (%lu bytes)\n", "Memory free:", (double)free_bytes / 1024 / 1024, free_bytes);
}