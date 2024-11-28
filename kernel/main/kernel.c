#include <debug.h>
#include <disk.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <net/network.h>
#include <paging.h>
#include <pci.h>
#include <pic.h>
#include <pit.h>
#include <printf.h>
#include <process.h>
#include <root_inode.h>
#include <scheduler.h>
#include <serial.h>
#include <syscall.h>
#include <vfs.h>
#include <vga_buffer.h>
#include <x86.h>

void display_grub_info(const multiboot_info_t *mbd, unsigned int magic);

#define STACK_CHK_GUARD 0xe2dee396
uint32_t wait_for_network_start;
uint32_t wait_for_network_timeout = 15'000;

uintptr_t __stack_chk_guard = STACK_CHK_GUARD; // NOLINT(*-reserved-identifier)

[[noreturn]] void panic(const char *msg)
{
    printf(KRED "\nKERNEL PANIC: " KWHT "%s\n", msg);
    debug_stats();

    while (1) {
        hlt();
    }

    __builtin_unreachable();
}

void wait_for_network()
{
    wait_for_network_start = scheduler_get_jiffies();
    sti();
    while (!network_is_ready() && scheduler_get_jiffies() - wait_for_network_start < wait_for_network_timeout) {
        hlt();
    }
    cli();
    if (!network_is_ready()) {
        printf("[ " KBRED "FAIL" KWHT " ] ");
        printf(KBYEL "Network failed to start\n" KWHT);
    }
}

void kernel_main(const multiboot_info_t *mbd, const uint32_t magic)
{
    cli();

    vga_buffer_init();
    init_serial();
    gdt_init();

    kernel_heap_init();
    paging_init();

    idt_init();
    pic_init();
    pit_init();
    display_grub_info(mbd, magic);
    init_symbols(mbd);
    scheduler_init();
    vfs_init();
    pci_scan();
    wait_for_network();
    disk_init();
    root_inode_init();

    register_syscalls();
    keyboard_init();
    scheduler_start();


    printf("\nStarting the shell");

    // stdout
    // vfs_open("/dev/tty", O_RDWR);

    start_shell(0);

    schedule();

    panic("Kernel finished");
}

void start_shell(const int console)
{
    struct process *process = nullptr;
    int res                 = process_load_enqueue("/bin/sh", &process);
    if (res < 0) {
        panic("Failed to load shell");
    }

    res = process_set_current_directory(process, "/");
    if (res < 0) {
        panic("Failed to set current directory");
    }

    process->priority = 1;
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

    /* Loop through the memory map and display the values */
    for (unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        const multiboot_memory_map_t *mmmt = (multiboot_memory_map_t *)(mbd->mmap_addr + i);

        const uint32_t type = mmmt->type;

        if (type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (mmmt->len > 0x100000) {
                printf("[ " KBGRN "OK" KWHT " ] ");
                printf("Available memory: %u MiB\n", (uint16_t)(mmmt->len / 1024 / 1024));
            }
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

    hlt();
}
