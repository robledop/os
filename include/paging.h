#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include "types.h"

// https://wiki.osdev.org/Paging

// PCD, is the 'Cache Disable' bit. If the bit is set, the page will not be cached.
// Otherwise, it will be.
#define PAGING_DIRECTORY_ENTRY_CACHE_DISABLED 0b00010000

// PWT, controls 'Write-Through' abilities of the page. If the bit is set, write-through
// caching is enabled. If not, then write-back is enabled instead.
#define PAGING_DIRECTORY_ENTRY_WRITE_THROUGH 0b00001000

// U/S, the 'User/Supervisor' bit, controls access to the page based on privilege level.
// If the bit is set, then the page may be accessed by all; if the bit is not set,
// however, only the supervisor can access it. For a page directory entry, the user bit
// controls access to all the pages referenced by the page directory entry. Therefore, if
// you wish to make a page a user page, you must set the user bit in the relevant page
// directory entry as well as the page table entry.
#define PAGING_DIRECTORY_ENTRY_SUPERVISOR 0b00000100

// R/W, the 'Read/Write' permissions flag. If the bit is set, the page is read/write.
// Otherwise, when it is not set, the page is read-only. The WP bit in CR0 determines
// if this is only applied to userland, always giving the kernel write access
// (the default) or both userland and the kernel (see Intel Manuals 3A 2-20).
#define PAGING_DIRECTORY_ENTRY_IS_WRITABLE 0b00000010

// P, or 'Present'. If the bit is set, the page is actually in physical memory at the
// moment. For example, when a page is swapped out, it is not in physical memory and
// therefore not 'Present'. If a page is called, but not present, a page fault will
// occur, and the OS should handle it.
#define PAGING_DIRECTORY_ENTRY_IS_PRESENT 0b00000001

// Clear all flags to unmap the page
#define PAGING_DIRECTORY_ENTRY_UNMAPPED 0x00

#define PAGING_ENTRIES_PER_TABLE 1024
#define PAGING_ENTRIES_PER_DIRECTORY 1024
#define PAGING_PAGE_SIZE 4096

// https://wiki.osdev.org/Paging#Page_Directory
struct page_directory {
    // https://wiki.osdev.org/Paging#Page_Table
    uint32_t *directory_entry;
};

struct page_directory *paging_create_directory(uint8_t flags);
void paging_free_directory(struct page_directory *page_directory);
void paging_switch_directory(const struct page_directory *directory);
// Defined in paging.asm
void enable_paging();
uint32_t *paging_get_directory(const struct page_directory *directory);

uint32_t paging_get(const struct page_directory *directory, void *virtual_address);
int paging_set(const struct page_directory *directory, void *virtual_address, uint32_t value);
bool paging_is_aligned(void *address);
int paging_map_to(const struct page_directory *directory, void *virtual_address, void *physical_start_address,
                  void *physical_end_address, int flags);
int paging_map_range(const struct page_directory *directory, void *virtual_address, void *physical_start_address,
                     int total_pages, int flags);
int paging_map(const struct page_directory *directory, void *virtual_address, void *physical_address, int flags);
void *paging_align_address(void *address);
void *paging_align_to_lower_page(void *address);
void *paging_get_physical_address(const struct page_directory *directory, void *virtual_address);
void paging_init();
