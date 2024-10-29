#include <debug.h>
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
#include <pic.h>
#include <pit.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <syscall.h>
#include <thread.h>
#include <vga_buffer.h>

void display_grub_info(const multiboot_info_t *mbd, unsigned int magic);

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD; // NOLINT(*-reserved-identifier)
int __cli_count             = 0;

extern struct page_directory *kernel_page_directory;

__attribute__((noreturn)) void panic(const char *msg)
{
    kprintf(KRED "\nKERNEL PANIC: " KWHT "%s\n", msg);
    debug_callstack();

    while (1) {
        asm volatile("hlt");
    }

    __builtin_unreachable();
}

/// @brief Set the kernel mode segments and switch to the kernel page directory
void kernel_page()
{
    set_kernel_mode_segments();
    paging_switch_directory(kernel_page_directory);
}

void kernel_main(multiboot_info_t *mbd, unsigned int magic)
{
    __stack_chk_guard  = STACK_CHK_GUARD;
    uint32_t stack_ptr = 0;
    asm("mov %%esp, %0" : "=r"(stack_ptr));

    ENTER_CRITICAL();

    init_serial();
    gdt_init(stack_ptr);
    vga_buffer_init();
    print(""); // BUG: Without this, the terminal gets all messed up, but only when using my bootloader
    terminal_clear();

    kprintf(KRESET KYEL "Kernel stack base: %x\n", stack_ptr);
    char *cpu = cpu_string();
    kprintf(KCYN "CPU: %s\n", cpu);
    cpu_print_info();
    kernel_heap_init();
    paging_init();
    idt_init();
    pic_init();
    pit_init();
    scheduler_init();
    display_grub_info(mbd, magic);
    pci_scan();
    fs_init();
    disk_init();
    register_syscalls();
    keyboard_init();

    start_shell(0);

    LEAVE_CRITICAL();

    panic("Kernel finished");
}

void start_shell(const int console)
{
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

    process->thread->tty = console;
    process->priority    = 1;

    // TODO: Save kernel state and switch to user mode
    schedule();
}

void display_grub_info(const multiboot_info_t *mbd, const unsigned int magic)
{
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

void system_reboot()
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

void system_shutdown()
{
    outw(0x604, 0x2000);

    asm volatile("hlt");
}
