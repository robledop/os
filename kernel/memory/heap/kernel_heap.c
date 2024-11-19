#include <config.h>
#include <debug.h>
#include <heap.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memory.h>
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
        warningf("Failed to create heap\n");
        panic("Failed to create heap\n");
    }
}

void *kmalloc(const size_t size)
{
    allocations++;
    // printf("Free blocks: %d\n", heap_count_free_blocks(&kernel_heap));
    void *result = heap_malloc(&kernel_heap, size);
    dbgprintf("kmalloc: %p\t", result);
    // stack_trace();
    return result;
}

void *kzalloc(const size_t size)
{
    void *ptr = kmalloc(size);
    if (!ptr) {
        warningf("Failed to allocate memory\n");
        ASSERT(false, "Failed to allocate memory");
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
    dbgprintf("kfree: %p\n", ptr);
    heap_free(&kernel_heap, ptr);
}

void kernel_heap_print_stats()
{
    printf("\nmalloc: %lu\n", allocations);
    printf("free: %lu\n", frees);
    printf("Free blocks: %lu\n", heap_count_free_blocks(&kernel_heap));
}