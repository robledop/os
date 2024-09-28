#ifndef PAGING_H
#define PAGING_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// https://wiki.osdev.org/Paging

#define PAGING_CACHE_DISABLED   0b00010000
#define PAGING_WRITE_THROUGH    0b00001000
#define PAGING_ACCESS_FROM_ALL  0b00000100
#define PAGING_IS_WRITABLE      0b00000010
#define PAGING_IS_PRESENT       0b00000001

#define PAGING_ENTRIES_PER_TABLE     1024
#define PAGING_ENTRIES_PER_DIRECTORY 1024
#define PAGING_PAGE_SIZE             4096

struct paging_4gb_chunk {
    uint32_t *directory_entry;
};

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags);
void paging_switch(struct paging_4gb_chunk *chunk);
void enable_paging();
uint32_t *paging_get_directory(struct paging_4gb_chunk *chunk);

int paging_set(uint32_t *directory, void *virtual_address, uint32_t value);
bool paging_is_aligned(void *address);

#endif