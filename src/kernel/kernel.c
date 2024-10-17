#include <disk.h>
#include <file.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <paging.h>
#include <pci.h>
#include <process.h>
#include <serial.h>
#include <string.h>
#include <syscall.h>
#include <task.h>
#include <terminal.h>
#include "console.h"

// Divide by zero error
extern void cause_problem();
void paging_demo();
void multitasking_demo();
void display_grub_info(const multiboot_info_t *mbd, unsigned int magic);

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD; // NOLINT(*-reserved-identifier)

extern struct page_directory *kernel_page_directory;

__attribute__((noreturn)) void panic(const char *msg) {
    kprintf(KRED "\nKERNEL PANIC: " KWHT "%s\n", msg);

    while (1) {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

void kernel_page() {
    kernel_registers();
    paging_switch_directory(kernel_page_directory);
}

void kernel_main(multiboot_info_t *mbd, unsigned int magic) {
    __stack_chk_guard  = STACK_CHK_GUARD;
    uint32_t stack_ptr = 0;
    asm("mov %%esp, %0" : "=r"(stack_ptr));

    disable_interrupts();

    init_serial();
    terminal_clear();
    kprintf(KCYN "Kernel stack base: %x\n", stack_ptr);
    char *cpu = cpu_string();
    kprintf(KCYN "CPU: %s\n", cpu);
    cpu_print_info();
    gdt_init(stack_ptr);
    kernel_heap_init();
    paging_init();
    idt_init();
    display_grub_info(mbd, magic);

    pci_scan();
    fs_init();
    disk_init();

    // my_fat16_init();

    register_syscalls();
    keyboard_init();

    initialize_consoles();

    enable_interrupts();


    panic("Kernel finished");
}

void start_shell(const int console) {
    dbgprintf("Loading shell\n");
    struct process *process = nullptr;
    int res                 = process_load_switch("0:/bin/sh", &process);
    if (res < 0) {
        panic("Failed to load shell");
    }

    res = process_set_current_directory(process, "0:/");
    if (res < 0) {
        panic("Failed to set current directory");
    }

    process->console = console;


    task_run_first_task();
}

void multitasking_demo() {
    struct process *process = nullptr;
    process_load_switch("0:/cblank.elf", &process);
    struct command_argument argument;
    strncpy(argument.argument, "Program 0", sizeof(argument.argument));
    argument.next = nullptr;
    process_inject_arguments(process, &argument);

    process_load_switch("0:/cblank.elf", &process);
    strncpy(argument.argument, "Program 1", sizeof(argument.argument));
    argument.next = nullptr;
    process_inject_arguments(process, &argument);
}

void paging_demo() {
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

void display_grub_info(const multiboot_info_t *mbd, const unsigned int magic) {
#ifdef GRUB
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("invalid magic number!");
    }

    /* Check bit 6 to see if we have a valid memory map */
    if (!(mbd->flags >> 6 & 0x1)) {
        panic("invalid memory map given by GRUB bootloader");
    }

    kprintf("Bootloader: %s\n", mbd->boot_loader_name);

    /* Loop through the memory map and display the values */
    for (unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        const multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *)(mbd->mmap_addr + i);

        const uint32_t type = mmmt->type;
        // kprintf("Start Addr: %x | Length: %x | Size: %x ",
        //         mmmt->addr, mmmt->len, mmmt->size);
        // kprintf("| Type %d\n", type);

        // dbgprintf("Start Addr: %x | Length: %x | Size: %x | Type: %x\n",
        //           mmmt->addr, mmmt->len, mmmt->size, type);

        if (type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmmt->len > 0x100000) {
                kprintf("Available memory: %d MiB\n", mmmt->len / 1024 / 1024);
            }
            /*
             * Do something with this memory block!
             * BE WARNED that some of the memory shown as available is actually
             * actively being used by the kernel! You'll need to take that
             * into account before writing to memory!
             */
        }
    }
#endif
}

void system_reboot() {
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

void system_shutdown() {
    outw(0x604, 0x2000);

    asm volatile("hlt");
}
