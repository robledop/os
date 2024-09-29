#include "kernel.h"
#include "idt/idt.h"
#include "io/io.h"
#include "terminal/terminal.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/stream.h"
#include "fs/file.h"

// Divide by zero error
extern void cause_problem();
void paging_demo();

static struct paging_4gb_chunk *kernel_chunk = 0;
void kernel_main()
{
    terminal_clear();
    kheap_init();
    fs_init();
    disk_search_and_init();
    idt_init();
    kernel_chunk = paging_new_4gb(
        PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
        PAGING_DIRECTORY_ENTRY_IS_PRESENT |
        PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch(kernel_chunk);

    enable_paging();

    // paging_demo();

    enable_interrupts();

    while (1)
    {
    }
}

void paging_demo()
{
    char *ptr1 = kzalloc(4096);

    ptr1[0] = 'C';
    ptr1[1] = 'D';
    paging_set(
        paging_get_directory(kernel_chunk),
        (void *)0x1000,
        (uint32_t)ptr1 |
            PAGING_DIRECTORY_ENTRY_IS_PRESENT |
            PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
            PAGING_DIRECTORY_ENTRY_SUPERVISOR);

    char *ptr2 = (char *)0x1000;

    print("*ptr1: ");
    print(hex_to_string((uint32_t)ptr1));
    print("\n");

    print("*ptr2: ");
    print(hex_to_string((uint32_t)ptr2));
    print("\n");

    print("ptr1 before: ");
    print(ptr1);
    print("\n");

    ptr2[0] = 'A';
    ptr2[1] = 'B';

    print("ptr1 after: ");
    print(ptr1);
    print("\n");

    print("ptr2: ");
    print(ptr2);
    print("\n");
}
