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
#include "ssp.h"

// Divide by zero error
extern void cause_problem();
void paging_demo();
void multitasking_demo();
void opendir_test();

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = 0xe2dee396;

extern struct page_directory *kernel_page_directory;

__attribute__((noreturn)) void panic(const char *msg)
{
    kprintf(KRED "KERNEL PANIC: " KWHT);
    kprintf("%s\n", msg);
    while (1)
    {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

void kernel_page()
{
    kernel_registers();
    paging_switch_directory(kernel_page_directory);
}

void kernel_main()
{
    __stack_chk_guard = 0xe2dee396;
    disable_interrupts();
    terminal_clear();
    kprintf(KCYN "Kernel is starting\n");
    init_serial();
    idt_init();
    init_gdt();
    kheap_init();

    paging_init();
    fs_init();
    disk_search_and_init();

    register_syscalls();

    keyboard_init();
    kprintf("Kernel is running\n");

    ///////////////////
    // opendir_test();
    ///////////////////

    struct process *process = NULL;
    int res = process_load_switch("0:/sh", &process);
    if (res < 0)
    {
        panic("Failed to load shell");
    }

    // enable_interrupts();

    task_run_first_task();

    panic("Kernel finished");
}

void opendir_test()
{
    struct file_directory directory = fs_open_dir("0:/");

    char *name = kmalloc(MAX_PATH_LENGTH);
    strncpy(name, directory.name, MAX_PATH_LENGTH);
    kprintf("Directory: %s\n", name);
    kfree(name);
    kprintf("Entries in directory: %d\n", directory.entry_count);
    for (int i = 0; i < directory.entry_count; i++)
    {
        struct directory_entry entry = directory.get_entry(directory.entries, i);
        if (entry.is_long_name)
        {
            continue;
        }
        if (strlen(entry.ext) > 0)
        {
            kprintf("%s.%s - dir: %d, ro: %d, h: %d, s: %d, v: %d\n",
                    entry.name,
                    entry.ext,
                    entry.is_directory,
                    entry.is_long_name,
                    entry.is_read_only,
                    entry.is_hidden,
                    entry.is_system,
                    entry.is_volume_label);
        }
        else
        {
            kprintf("%s - dir: %d, ro: %d, h: %d, s: %d, v: %d\n",
                    entry.name,
                    entry.is_directory,
                    entry.is_long_name,
                    entry.is_read_only,
                    entry.is_hidden,
                    entry.is_system,
                    entry.is_volume_label);
        }
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
