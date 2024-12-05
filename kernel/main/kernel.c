#include <config.h>
#include <debug.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <keyboard.h>
#include <memory.h>
#include <paging.h>
#include <pic.h>
#include <pit.h>
#include <printf.h>
#include <root_inode.h>
#include <serial.h>
#include <syscall.h>
#include <task.h>
#include <timer.h>
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
    // debug_stats();

    while (1) {
        hlt();
    }

    __builtin_unreachable();
}

void idle()
{
    while (true) {
        // printf(KBMAG ".");
        // sti();
        hlt();
    }
}

void yellow()
{
    for (int i = 0; i < 10; i++) {
        // int i = 0;
        // while (true) {
        printf(KBYEL ".");
    }
}

void cyan()
{
    for (int i = 0; i < 10; i++) {
        // int i = 0;
        // while (true) {
        printf(KBCYN ".");
    }
}

// Set up first user process.
// void user_init(void)
// {
//     extern char _binary___build_apps_blank_start[], _binary___build_apps_blank_size[]; //
//     NOLINT(*-reserved-identifier)
//
//     char *program_start = _binary___build_apps_blank_start;
//     size_t program_size = (size_t)_binary___build_apps_blank_size;
//
//     struct task *new_task = tasks_new(nullptr, nullptr, TASK_PAUSED, "init", USER_MODE);
//     void *program         = kzalloc(program_size);
//     memcpy(program, program_start, program_size);
//     paging_map_to(new_task->page_directory,
//                   (void *)PROGRAM_VIRTUAL_ADDRESS,
//                   program,
//                   paging_align_address((char *)program + program_size),
//                   PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
//                       PAGING_DIRECTORY_ENTRY_SUPERVISOR);
//     void *user_stack = kzalloc(USER_STACK_SIZE);
//     paging_map_to(new_task->page_directory,
//                   (void *)USER_STACK_BOTTOM,
//                   user_stack,
//                   paging_align_address((char *)user_stack + USER_STACK_SIZE),
//                   PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
//                       PAGING_DIRECTORY_ENTRY_SUPERVISOR);
//     tasks_unblock(new_task);
// }


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
    timer_init(1000);
    display_grub_info(mbd, magic);
    init_symbols(mbd);
    vfs_init();
    disk_init();
    root_inode_init();
    register_syscalls();
    keyboard_init();

    tasks_init();

    struct task idle_task;
    idle_task.priority = -1;
    current_task       = tasks_new(idle, &idle_task, TASK_PAUSED, "idle", KERNEL_MODE);
    tasks_set_idle_task(current_task);

    start_shell(0);

    tasks_block_current(TASK_PAUSED);

    panic("Kernel terminated");
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

    tasks_unblock(process->thread);
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
