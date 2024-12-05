#include "heap.h"

// #include <debug.h>

#include <assert.h>
#include <kernel.h>


#include "memory.h"
#include "serial.h"
#include "status.h"

// https://wiki.osdev.org/Memory_Map_(x86)

// Validate the table of a heap
// ptr: The start address of the heap
// end: The end address of the heap
// table: The table that will be used to manage the heap
static int heap_validate_table(void *ptr, void *end, const struct heap_table *table)
{
    int res = 0;

    const size_t table_size   = (size_t)((char *)end - (char *)ptr);
    const size_t total_blocks = table_size / HEAP_BLOCK_SIZE;

    // If the total number of blocks is not a multiple of HEAP_BLOCK_SIZE, return an error
    if (table->total != total_blocks) {
        warningf("Invalid table size\n");
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

// Validate the alignment of a pointer
// ptr: The pointer to validate
// It must be a multiple of HEAP_BLOCK_SIZE
static bool heap_validate_alignment(void *ptr)
{
    return (uintptr_t)ptr % HEAP_BLOCK_SIZE == 0;
}

// Create a new heap
// ptr: The start address of the heap
// end: The end address of the heap
// table: The table that will be used to manage the heap
int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table)
{
    int res = 0;

    // Validate the alignment of the start and end pointers
    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
        warningf("Invalid alignment\n");
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->start = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0) {
        warningf("Failed to validate table\n");
        goto out;
    }

    const size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_FREE, table_size);

out:
    return res;
}

// Align a value to the next multiple of HEAP_BLOCK_SIZE
static uint32_t heap_align_value_to_upper(uint32_t value)
{
    // If the value is already aligned, return it
    if (value % HEAP_BLOCK_SIZE == 0) {
        return value;
    }

    // Align the value to the next multiple of HEAP_BLOCK_SIZE
    value = value + HEAP_BLOCK_SIZE - (value % HEAP_BLOCK_SIZE);
    return value;
}

// Convert a block number to an address
static void *heap_block_to_address(const struct heap *heap, const uint32_t block)
{
    return (void *)((uintptr_t)heap->start + block * HEAP_BLOCK_SIZE);
}

// Mark a range of blocks as taken
// heap: The heap to mark the blocks in
// start_block: The block number of the first block to mark
// blocks_needed: The number of blocks to mark as taken
static void heap_mark_blocks_taken(const struct heap *heap, const uint32_t start_block, const uint32_t blocks_needed)
{
    const uint32_t end_block = (start_block + blocks_needed) - 1;
    // Mark the first block as taken
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TAKEN | HEAP_BLOCK_IS_FIRST;

    // If more than one block is needed, mark the first block as having a next block
    if (blocks_needed > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    // Mark the rest of the blocks as taken
    for (uint32_t i = start_block; i <= end_block; i++) {
        // printf(KBYEL "." KRESET);
        heap->table->entries[i] = entry;
        entry                   = HEAP_BLOCK_TAKEN;
        // If this is not the last block, mark it as having a next block
        if (i != end_block - 1) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

// Get the type of a heap block entry
// The type can be one of the following:
// - HEAP_BLOCK_TAKEN
// - HEAP_BLOCK_FREE
static int heap_get_entry_type(const HEAP_BLOCK_TABLE_ENTRY entry)
{
    // Currently the type is stored in the least significant bit
    return entry & 0b00001111;
}

/// @param heap the heap to search in
/// @param blocks_needed the number of blocks needed
/// @return the block number of the first block that can hold the requested number of blocks
static int heap_get_start_block(const struct heap *heap, const uint32_t blocks_needed)
{
    ASSERT(heap != nullptr);
    const struct heap_table *table = heap->table;
    // The block number of the first block that can hold the requested number of blocks
    int start_block = -1;
    // The number of free blocks found so far
    uint32_t free_blocks = 0;

    // Search for the first block that can hold the requested number of blocks
    for (size_t i = 0; i < table->total; i++) {
        // If the block is not free, reset the free block counter
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_FREE) {
            free_blocks = 0;
            start_block = -1;
            continue;
        }

        // If this is the first free block
        if (start_block == -1) {
            start_block = (int)i;
        }

        free_blocks++;
        // If the number of free blocks found so far is equal to the number of blocks needed
        if (free_blocks == blocks_needed) {
            break;
        }
    }

    // If no free blocks were found, return an error
    if (start_block == -1) {
        warningf("No free blocks found\n");
        return -ENOMEM;
    }

    return start_block;
}

// Allocate a number of blocks in the heap
void *heap_malloc_blocks(const struct heap *heap, const uint32_t blocks_needed)
{
    void *address         = nullptr;
    const int start_block = heap_get_start_block(heap, blocks_needed);

    // If no free blocks were found, return
    if (start_block < 0) {
        panic("Failed to find free blocks\n");
        return address;
    }

    address = heap_block_to_address(heap, start_block);

    heap_mark_blocks_taken(heap, start_block, blocks_needed);

    return address;
}

uint32_t heap_count_free_blocks(const struct heap *heap)
{
    const struct heap_table *table = heap->table;
    uint32_t free_blocks           = 0;

    for (size_t i = 0; i < table->total; i++) {
        if (heap_get_entry_type(table->entries[i]) == HEAP_BLOCK_FREE) {
            free_blocks++;
        }
    }

    return free_blocks;
}

// Mark a range of blocks as free
// heap: The heap to mark the blocks in
void heap_mark_blocks_free(const struct heap *heap, const uint32_t start_block)
{
    const struct heap_table *table = heap->table;
    for (size_t i = start_block; i < table->total; i++) {
        const HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i]                  = HEAP_BLOCK_FREE;
        // printf(KBBLU "." KRESET);

        // If the block has no next block, stop
        if ((entry & HEAP_BLOCK_HAS_NEXT) != HEAP_BLOCK_HAS_NEXT) {
            break;
        }
    }
}


// Convert an address to a block number
uint32_t heap_address_to_block(const struct heap *heap, void *address)
{
    return ((uint32_t)address - (uint32_t)heap->start) / HEAP_BLOCK_SIZE;
}

uint32_t heap_number_of_blocks(const struct heap *heap, void *ptr)
{
    const uint32_t start_block     = heap_address_to_block(heap, ptr);
    const struct heap_table *table = heap->table;
    uint32_t blocks                = 1;

    for (size_t i = start_block; i < table->total; i++) {
        const HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
        blocks++;
    }

    return blocks;
}

void *heap_malloc(const struct heap *heap, const size_t size)
{
    const size_t aligned_size    = heap_align_value_to_upper(size);
    const uint32_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;

    return heap_malloc_blocks(heap, blocks_needed);
}

void *heap_realloc(const struct heap *heap, void *ptr, const size_t size)
{
    const size_t aligned_size    = heap_align_value_to_upper(size);
    const uint32_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;
    void *new_ptr                = heap_malloc_blocks(heap, blocks_needed);

    const uint32_t old_blocks = heap_number_of_blocks(heap, ptr);

    memcpy(new_ptr, ptr, old_blocks * HEAP_BLOCK_SIZE);

    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));

    return new_ptr;
}

void heap_free(const struct heap *heap, void *ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}