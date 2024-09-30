#include "kernel.h"
#include "idt.h"
#include "io.h"
#include "terminal.h"
#include "kheap.h"
#include "paging.h"
#include "string.h"
#include "disk.h"
#include "pparser.h"
#include "stream.h"
#include "file.h"
#include "serial.h"

// Divide by zero error
extern void cause_problem();
void paging_demo();
void fs_demo();

static struct paging_4gb_chunk *kernel_chunk = 0;

void panic(const char *msg)
{
    warningf("KERNEL PANIC: %s\n", msg);
    kprint(KRED "KERNEL PANIC:" KWHT);
    kprint("%s\n", msg);
    while (1)
    {
        asm volatile("hlt");
    }
}

void kernel_main()
{
    init_serial();
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
    enable_interrupts();

    kprint(KCYN"Kernel is running\n");
    dbgprintf("Kernel is running");

    while (1)
    {
        asm volatile("hlt");
    }
}

void fs_demo()
{
    int fd = fopen("0:/test/hello.txt", "r");
    if (fd)
    {
        dbgprintf("File opened\n");
        char buffer[14];
        // fseek(fd, 2, SEEK_SET);
        fread(buffer, 13, 1, fd);
        buffer[13] = 0x00;
        dbgprintf("File contents: %s\n", buffer);

        struct file_stat stat;
        fstat(fd, &stat);

        dbgprintf("File size: %d bytes\n", stat.size);

        dbgprintf("File flags: %s\n", hex_to_string(stat.flags));

        if (fclose(fd) == 0)
        {
            dbgprintf("File closed");
        }
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

    dbgprintf("*ptr1: %x\n", ptr1);
    dbgprintf("*ptr2: %x\n", ptr2);
    dbgprintf("ptr1 before: %x\n", ptr1);

    ptr2[0] = 'A';
    ptr2[1] = 'B';

    dbgprintf("ptr1 after: %s\n", ptr1);
    dbgprintf("ptr2: %s\n", ptr2);
}
