#include <kernel.h>
#include <ata.h>
#include <config.h>
#include <file.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
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
#include <syscall.h>
#include "fat16.h"

////////////////////////////////////////////////////////////
// For internal use
typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1
struct fat_directory_entry
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

struct fat_directory
{
    struct fat_directory_entry *entries;
    int entry_count;
    int sector_position;
    int ending_sector_position;
};

struct fat_item
{
    union
    {
        struct fat_directory_entry *item;
        struct fat_directory *directory;
    };
    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor
{
    struct fat_item *item;
    uint32_t position;
};

//////////////////////////////////////////////////////////////

// Divide by zero error
extern void cause_problem();
void paging_demo();
void fs_demo();
void multitasking_demo();

static struct page_directory *kernel_page_directory = 0;

void panic(const char *msg)
{
    kprintf(KRED "KERNEL PANIC: " KWHT);
    kprintf("%s\n", msg);
    while (1)
    {
        asm volatile("hlt");
    }
}

void kernel_page()
{
    // dbgprintf("Switching to kernel page\n");
    kernel_registers();
    paging_switch_directory(kernel_page_directory);
}

// struct tss tss;
// struct gdt gdt_real[TOTAL_GDT_SEGMENTS];
// struct gdt_structured gdt_structured[TOTAL_GDT_SEGMENTS] = {
//     {.base = 0x00, .limit = 0x00, .type = 0x00},                  // NULL
//     {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x9A},            // Kernel code
//     {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0x92},            // Kernel data
//     {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF8},            // User code
//     {.base = 0x00, .limit = 0xFFFFFFFF, .type = 0xF2},            // User data
//     {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9}, // TSS
// };

void kernel_main()
{
    idt_init();
    init_serial();
    terminal_clear();
    kprintf(KCYN "Kernel is starting\n");
    // memset(gdt_real, 0, sizeof(gdt_real));
    // gdt_structured_to_gdt(gdt_real, gdt_structured, TOTAL_GDT_SEGMENTS);
    // // dbgprintf("Loading GDT\n");

    // gdt_load(gdt_real, sizeof(gdt_real));

    kheap_init();
    fs_init();
    disk_search_and_init();

    // dbgprintf("Initializing the TSS \n");
    // memset(&tss, 0, sizeof(tss));
    // tss.esp0 = 0x60000; // Kernel stack
    // tss.ss0 = DATA_SELECTOR;
    // kprintf("Kernel stack address: %x\n", tss.esp0);
    // kprintf("TSS address: %x\n", &tss);
    // kprintf("TSS ss0: %x\n", tss.ss0);
    // kprintf("TSS fs: %x\n", tss.fs);

    // tss_load(0x28);

    kernel_page_directory = paging_create_directory(
        PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(kernel_page_directory);

    init_gdt();
    init_tss();

    enable_paging();

    terminal_clear();
    kprintf("Kernel is running\n");
    register_syscalls();
    keyboard_init();

    // struct file_directory directory = fs_open_dir("0:/test");

    // kprintf("Files in root directory: %d\n", directory.entry_count);
    // for (int i = 0; i < directory.entry_count; i++)
    // {
    //     struct directory_entry entry = directory.get_entry(directory.entries, i);
    //     if (entry.is_long_name)
    //     {
    //         continue;
    //     }
    //     if (strlen(entry.ext) > 0)
    //     {
    //         kprintf("%s.%s - dir: %d, ro: %d, h: %d, s: %d, v: %d\n",
    //                 entry.name,
    //                 entry.ext,
    //                 entry.is_directory,
    //                 entry.is_long_name,
    //                 entry.is_read_only,
    //                 entry.is_hidden,
    //                 entry.is_system,
    //                 entry.is_volume_label);
    //     }
    //     else
    //     {
    //         kprintf("%s - dir: %d, ro: %d, h: %d, s: %d, v: %d\n",
    //                 entry.name,
    //                 entry.is_directory,
    //                 entry.is_long_name,
    //                 entry.is_read_only,
    //                 entry.is_hidden,
    //                 entry.is_system,
    //                 entry.is_volume_label);
    //     }
    // }

    struct process *process = NULL;
    int res = process_load_switch("0:/sh", &process);
    if (res < 0)
    {
        panic("Failed to load shell");
    }

    task_run_first_task();

    // enable_interrupts();

    while (1)
    {
        asm volatile("hlt");
    }
}

void multitasking_demo()
{
    struct process *process = NULL;
    process_load_switch("0:/cblank.elf", &process);
    struct command_argument argument;
    strncpy(argument.argument, "Program 0", sizeof(argument.argument));
    argument.next = NULL;
    process_inject_arguments(process, &argument);

    process_load_switch("0:/cblank.elf", &process);
    strncpy(argument.argument, "Program 1", sizeof(argument.argument));
    argument.next = NULL;
    process_inject_arguments(process, &argument);
}

void fs_demo()
{
    int fd = fopen("0:/hello.txt", "r");
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

        // dbgprintf("File flags: %x\n", stat.flags);

        if (fclose(fd) == 0)
        {
            dbgprintf("File closed\n");
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
