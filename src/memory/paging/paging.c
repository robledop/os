#include "paging.h"
#include "assert.h"
#include "kernel_heap.h"
#include "serial.h"
#include "status.h"

// https://wiki.osdev.org/Paging

struct page_directory *kernel_page_directory = nullptr;

static uint32_t *current_directory = nullptr;
void paging_load_directory(uint32_t *directory);

bool paging_is_video_memory(const uint32_t address)
{
    return address >= 0xB8000 && address <= 0xBFFFF;
}

struct page_directory *paging_create_directory(const uint8_t flags)
{
    dbgprintf("Allocating page directory. Present: %d, Write: %d, Supervisor: %d \n",
              flags & PAGING_DIRECTORY_ENTRY_IS_PRESENT,
              (flags & PAGING_DIRECTORY_ENTRY_IS_WRITABLE) >> 1,
              (flags & PAGING_DIRECTORY_ENTRY_SUPERVISOR) >> 2);

    uint32_t *directory_entry = kzalloc(sizeof(uint32_t) * PAGING_ENTRIES_PER_DIRECTORY);
    uint32_t offset           = 0;
    for (size_t i = 0; i < PAGING_ENTRIES_PER_TABLE; i++) {
        // The table is freed in paging_free_directory
        // ReSharper disable once CppDFAMemoryLeak
        uint32_t *table = kzalloc(sizeof(uint32_t) * PAGING_ENTRIES_PER_TABLE);
        for (size_t j = 0; j < PAGING_ENTRIES_PER_TABLE; j++) {
            table[j] = (offset + (j * PAGING_PAGE_SIZE)) | flags;
        }
        offset             = offset + (PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        directory_entry[i] = (uint32_t)table | flags | PAGING_DIRECTORY_ENTRY_IS_WRITABLE;
    }

    struct page_directory *directory = kzalloc(sizeof(struct page_directory));
    directory->directory_entry       = directory_entry;
    dbgprintf("Page directory allocated at %x\n", directory->directory_entry);
    return directory;
}

void paging_free_directory(struct page_directory *page_directory)
{
    if (!page_directory) {
        warningf("Page directory is null\n");
        return;
    }

    dbgprintf("Freeing page directory %x\n", &page_directory);

    for (int i = 0; i < PAGING_ENTRIES_PER_DIRECTORY; i++) {
        const uint32_t entry = page_directory->directory_entry[i];
        auto const table     = (uint32_t *)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(page_directory->directory_entry);
    kfree(page_directory);
}

// Switch page directory
void paging_switch_directory(const struct page_directory *directory)
{
    ASSERT(directory->directory_entry != nullptr, "Page directory is null");
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

uint32_t *paging_get_directory(const struct page_directory *directory)
{
    return directory->directory_entry;
}

bool paging_is_aligned(void *address)
{
    return ((uint32_t)address % PAGING_PAGE_SIZE) == 0;
}

int paging_get_indexes(void *virtual_address, uint32_t *directory_index_out, uint32_t *table_index_out)
{
    int res = 0;
    if (!paging_is_aligned(virtual_address)) {
        ASSERT(false, "Virtual address is not aligned");
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = ((uint32_t)virtual_address / (PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index_out = ((uint32_t)virtual_address % (PAGING_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE)) / PAGING_PAGE_SIZE;

out:
    return res;
}

// Align the address to the next page
void *paging_align_address(void *address)
{
    if (paging_is_aligned(address)) {
        return address;
    }

    return (void *)((uint32_t)address + PAGING_PAGE_SIZE - ((uint32_t)address % PAGING_PAGE_SIZE));
}

// Align the address to the lower page
void *paging_align_to_lower_page(void *address)
{
    return (void *)((uint32_t)address - ((uint32_t)address % PAGING_PAGE_SIZE));
}

int paging_map(const struct page_directory *directory, void *virtual_address, void *physical_address, const int flags)
{
    ASSERT(!paging_is_video_memory((uint32_t)physical_address), "Trying to map video memory");
    dbgprintf("Mapping virtual address %x to physical address %x\n", virtual_address, physical_address);

    if (!paging_is_aligned(virtual_address)) {
        warningf("Virtual address %x is not page aligned\n", (uint32_t)virtual_address);
        ASSERT(false, "Virtual address is not aligned");
        return -EINVARG;
    }

    if (!paging_is_aligned(physical_address)) {
        warningf("Physical address %x is not page aligned\n", (uint32_t)physical_address);
        ASSERT(false, "Physical address is not aligned");
        return -EINVARG;
    }

    return paging_set(directory, virtual_address, (uint32_t)physical_address | flags);
}

int paging_map_range(const struct page_directory *directory, void *virtual_address, void *physical_start_address,
                     const int total_pages, const int flags)
{
    ASSERT(!paging_is_video_memory((uint32_t)physical_start_address), "Trying to map video memory");
    int res = 0;

    for (int i = 0; i < total_pages; i++) {
        res = paging_map(directory, virtual_address, physical_start_address, flags);
        if (res < 0) {
            warningf("Failed to map page %d\n", i);
            ASSERT(false, "Failed to map page");
            break;
        }

        virtual_address        = (char *)virtual_address + PAGING_PAGE_SIZE;
        physical_start_address = (char *)physical_start_address + PAGING_PAGE_SIZE;
    }

    return res;
}

int paging_map_to(const struct page_directory *directory, void *virtual_address, void *physical_start_address,
                  void *physical_end_address, const int flags)
{
    ASSERT(!paging_is_video_memory((uint32_t)physical_start_address), "Trying to map video memory");
    ASSERT(!paging_is_video_memory((uint32_t)physical_end_address), "Trying to map video memory");
    int res = 0;

    if ((uint32_t)virtual_address % PAGING_PAGE_SIZE) {
        warningf("Virtual address %x is not page aligned\n", (uint32_t)virtual_address);
        ASSERT(false, "Virtual address is not aligned");
        res = -EINVARG;
    }

    if (!paging_is_aligned(physical_start_address)) {
        warningf("Physical start address %x is not page aligned\n", (uint32_t)physical_start_address);
        ASSERT(false, "Physical start address is not aligned");
        res = -EINVARG;
    }

    if (!paging_is_aligned(physical_end_address)) {
        warningf("Physical end address %x is not page aligned\n", (uint32_t)physical_end_address);
        ASSERT(false, "Physical end address is not aligned");
        res = -EINVARG;
    }

    if ((uint32_t)physical_end_address < (uint32_t)physical_start_address) {
        warningf("Physical end address %x is less than physical start address %x\n",
                 (uint32_t)physical_end_address,
                 (uint32_t)physical_start_address);
        ASSERT(false, "Physical end address is less than physical start address");
        res = -EINVARG;
    }

    const int total_bytes = (char *)physical_end_address - (char *)physical_start_address;
    const int total_pages = total_bytes / PAGING_PAGE_SIZE;
    res                   = paging_map_range(directory, virtual_address, physical_start_address, total_pages, flags);

    return res;
}

void *paging_get_physical_address(const struct page_directory *directory, void *virtual_address)
{
    auto const virt_address_new = (void *)paging_align_to_lower_page(virtual_address);
    auto const offset           = (void *)((uint32_t)virtual_address - (uint32_t)virt_address_new);

    return (void *)((paging_get(directory, virt_address_new) & 0xFFFFF000) + (uint32_t)offset);
}

// Get the physical address of a page
uint32_t paging_get(const struct page_directory *directory, void *virtual_address)
{
    uint32_t directory_index = 0;
    uint32_t table_index     = 0;
    const int res            = paging_get_indexes(virtual_address, &directory_index, &table_index);
    if (res < 0) {
        ASSERT(false, "Failed to get indexes");
        return 0;
    }

    const uint32_t entry  = directory->directory_entry[directory_index];
    const uint32_t *table = (uint32_t *)(entry & 0xFFFFF000); // get the address without the flags
    return table[table_index];
}

int paging_set(const struct page_directory *directory, void *virtual_address, const uint32_t value)
{
    dbgprintf("Setting page at virtual address %x to value %x\n", virtual_address, value);

    if (!paging_is_aligned(virtual_address)) {
        warningf("Virtual address %x is not page aligned\n", (uint32_t)virtual_address);
        ASSERT(false, "Virtual address is not aligned");
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index     = 0;
    const int res            = paging_get_indexes(virtual_address, &directory_index, &table_index);
    if (res < 0) {
        warningf("Failed to get indexes\n");
        ASSERT(false, "Failed to get indexes");
        return res;
    }

    const uint32_t entry = directory->directory_entry[directory_index];
    auto const table     = (uint32_t *)(entry & 0xFFFFF000); // The address without the flags
    table[table_index]   = value;

    return 0;
}

void paging_init()
{
    kernel_page_directory = paging_create_directory(
        PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(kernel_page_directory);
    enable_paging();
}