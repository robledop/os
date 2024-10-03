#include <kernel.h>
#include <ata.h>
#include <config.h>
#include <file.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <isr80h.h>
#include <keyboard.h>
#include <kheap.h>
#include <memory.h>
#include <paging.h>
#include <pparser.h>
#include <process.h>
#include <serial.h>
#include <status.h>
#include <stream.h>
#include <string.h>
#include <task.h>
#include <terminal.h>
#include <tss.h>

// Divide by zero error
extern void cause_problem();
void paging_demo();
void fs_demo();

static struct page_directory *kernel_page_directory = 0;

void panic(const char *msg)
{
    warningf("KERNEL PANIC: %s\n", msg);
    kprintf(KRED "KERNEL PANIC: " KWHT);
    kprintf("%s\n", msg);
    while (1)
    {
        asm volatile("hlt");
    }
}

void kernel_page()
{
    dbgprintf("Switching to kernel page\n");
    kernel_registers();
    paging_switch_directory(kernel_page_directory);
}

struct tss tss;
struct gdt gdt_real[TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                  // NULL
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A},            // Kernel code
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92},            // Kernel data
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8},            // User code
    {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF2},            // User data
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9}, // TSS
};

void kernel_main()
{
    init_serial();
    terminal_clear();
    kprintf(KCYN "Kernel is starting\n");
    memset(gdt_real, 0, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured, TOTAL_GDT_SEGMENTS);
    dbgprintf("Loading GDT\n");

    gdt_load(gdt_real, sizeof(gdt_real));

    kheap_init();
    fs_init();
    disk_search_and_init();
    idt_init();

    dbgprintf("Initializing the TSS \n");
    memset(&tss, 0, sizeof(tss));
    tss.esp0 = 0x60000; // Kernel stack
    tss.ss0 = DATA_SELECTOR;
    dbgprintf("Kernel stack address: %x\n", tss.esp0);

    dbgprintf("Loading the TSS\n");
    tss_load(0x28);

    kernel_page_directory = paging_create_directory(
        PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(kernel_page_directory);
    enable_paging();

    kprintf(KCYN "Kernel is running\n");
    dbgprintf("Kernel is running\n");
    isr80h_register_commands();
    keyboard_init();

    struct process *process = 0;
    int res = process_load_switch("0:/blank.elf", &process);
    if (res != ALL_OK)
    {
        panic("Failed to load process");
    }

    task_run_first_ever_task();

    enable_interrupts();

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

        dbgprintf("File flags: %x\n", stat.flags);

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
    paging_set(kernel_page_directory, (void *)0x1000,
               (uint32_t)ptr1 | PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
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
