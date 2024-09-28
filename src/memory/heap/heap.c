#include "heap.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

// https://wiki.osdev.org/Memory_Map_(x86)

static int heap_validate_table(void *ptr, void *end, struct heap_table *table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / HEAP_BLOCK_SIZE;

    if (table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

static bool heap_validate_alignment(void *ptr)
{
    return (unsigned int)ptr % HEAP_BLOCK_SIZE == 0;
}

int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table)
{
    int res = 0;

    res = -EIO;

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->start = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static uint32_t heap_align_value_to_upper(uint32_t value)
{
    if (value % HEAP_BLOCK_SIZE == 0)
    {
        return value;
    }

    value = value + HEAP_BLOCK_SIZE - (value % HEAP_BLOCK_SIZE);
    return value;
}

static void *heap_block_to_address(struct heap *heap, uint32_t block)
{
    return (void *)((uintptr_t)heap->start + block * HEAP_BLOCK_SIZE);
}

static void heap_mark_blocks_taken(struct heap *heap, uint32_t start_block, uint32_t blocks_needed)
{
    int end_block = (start_block + blocks_needed) - 1;
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (blocks_needed > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (int i = start_block; i <= end_block + blocks_needed; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != end_block - 1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0b00001111;
}

static int heap_get_start_block(struct heap *heap, uint32_t blocks_needed)
{
    struct heap_table *table = heap->table;
    int start_block = -1;
    int free_blocks = 0;

    for (int i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            free_blocks = 0;
            start_block = -1;
            continue;
        }

        // If this is the first free block
        if (start_block == -1)
        {
            start_block = i;
        }

        free_blocks++;
        if (free_blocks == blocks_needed)
        {
            break;
        }
    }

    if (start_block == -1)
    {
        return -ENOMEM;
    }

    return start_block;
}

void *heap_malloc_blocks(struct heap *heap, uint32_t blocks_needed)
{
    void *address = 0;
    int start_block = heap_get_start_block(heap, blocks_needed);

    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    // Mark blocks as taken
    heap_mark_blocks_taken(heap, start_block, blocks_needed);

out:
    return address;
}

void heap_mark_blocks_free(struct heap *heap, uint32_t start_block)
{
    struct heap_table *table = heap->table;
    for (int i = start_block; i < (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

int heap_address_to_block(struct heap *heap, void *address)
{
    return ((uintptr_t)address - (uintptr_t)heap->start) / HEAP_BLOCK_SIZE;
}

void *heap_malloc(struct heap *heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;

    return heap_malloc_blocks(heap, blocks_needed);
}

void heap_free(struct heap *heap, void *ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}