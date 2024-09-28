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

// Divide by zero error
extern void cause_problem();
void paging_demo();

static struct paging_4gb_chunk *kernel_chunk = 0;
void kernel_main()
{
    terminal_clear();

    print("Initializing kernel heap\n");
    kheap_init();

    print("Initializing disk\n");
    disk_search_and_init();

    print("Initializing interrupt descriptor table\n");
    idt_init();

    print("Allocating kernel memory chunk\n");
    kernel_chunk = paging_new_4gb(
        PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
        PAGING_DIRECTORY_ENTRY_IS_PRESENT |
        PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    print("Switching paging to kernel memory chunk\n");
    paging_switch(kernel_chunk);

    enable_paging();

    // paging_demo();

    print("Enabling interrupts\n");
    enable_interrupts();

    struct disk_stream *stream = disk_stream_create(0);
    disk_stream_seek(stream, 0x201);
    unsigned char c = 0;
    disk_stream_read(stream, &c, 1);
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
