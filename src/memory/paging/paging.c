#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "string/string.h"
#include "terminal/terminal.h"

// https://wiki.osdev.org/Paging

static uint32_t *current_directory = 0;
void paging_load_directory(uint32_t *directory);

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags)
{
    print("Allocating kernel memory chunk\n");
    uint32_t *directory = kzalloc(sizeof(uint32_t) * PAGING_ENTRIES_PER_DIRECTORY);
    int offset = 0;
    for (size_t i = 0; i < PAGING_ENTRIES_PER_TABLE; i++)
    {
        uint32_t *entry = kzalloc(sizeof(uint32_t) * PAGING_ENTRIES_PER_TABLE);
        for (size_t j = 0; j < PAGING_ENTRIES_PER_TABLE; j++)
        {
            entry[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
        }
        offset += PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE;
        directory[i] = (uint32_t)entry | flags | PAGING_DIRECTORY_ENTRY_IS_WRITABLE;
    }

    struct paging_4gb_chunk *chunk = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk->directory_entry = directory;
    return chunk;
}

void paging_switch(struct paging_4gb_chunk *chunk)
{
    print("Switching paging to kernel memory chunk\n");
    paging_load_directory(chunk->directory_entry);
    current_directory = chunk->directory_entry;
}

uint32_t *paging_get_directory(struct paging_4gb_chunk *chunk)
{
    return chunk->directory_entry;
}

bool paging_is_aligned(void *address)
{
    return ((uint32_t)address % PAGING_PAGE_SIZE) == 0;
}

int paging_get_indexes(void *virtual_address, uint32_t *directory_index_out, uint32_t *table_index_out)
{
    int res = 0;
    if (!paging_is_aligned(virtual_address))
    {
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = ((uint32_t)virtual_address / (PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index_out = ((uint32_t)virtual_address % (PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE)) / PAGING_PAGE_SIZE;

out:
    return res;
}

int paging_set(uint32_t *directory, void *virtual_address, uint32_t value)
{
    print("Setting page at virtual address ");
    print(hex_to_string((uint32_t)virtual_address));
    print(" to value ");
    print(hex_to_string(value));
    print("\n");
    if (!paging_is_aligned(virtual_address))
    {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virtual_address, &directory_index, &table_index);
    if (res < 0)
    {
        return res;
    }

    uint32_t entry = directory[directory_index];
    uint32_t *table = (uint32_t *)(entry & 0xFFFFF000);
    table[table_index] = value;

    return 0;
}