#include "idt.h"

#include <scheduler.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "io.h"
#include "kernel.h"
#include "memory.h"
#include "serial.h"
#include "status.h"
#include "thread.h"
#include "vga_buffer.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table
typedef void (*INTERRUPT_HANDLER_FUNCTION)(void);

extern INTERRUPT_HANDLER_FUNCTION interrupt_pointer_table[TOTAL_INTERRUPTS];
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[TOTAL_INTERRUPTS];
static SYSCALL_HANDLER_FUNCTION syscalls[MAX_SYSCALLS];
extern void idt_load(struct idtr_desc *ptr);
extern void isr80h_wrapper();

char *exception_messages[] = {"Division By Zero",
                              "Debug",
                              "Non Maskable Interrupt",
                              "Breakpoint",
                              "Into Detected Overflow",
                              "Out of Bounds",
                              "Invalid Opcode",
                              "No Coprocessor",
                              "Double Fault",
                              "Coprocessor Segment Overrun",
                              "Bad TSS",
                              "Segment Not Present",
                              "Stack Fault",
                              "General Protection Fault",
                              "Page Fault",
                              "Unknown Interrupt",
                              "x87 FPU Floating-Point Error",
                              "Alignment Check",
                              "Machine Check",
                              "SIMD Floating-Point Exception",
                              "Virtualization Exception",
                              "Control Protection Exception",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Reserved",
                              "Security Exception",
                              "Reserved"};

struct idt_desc idt_descriptors[TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

void no_interrupt_handler(const int interrupt, const uint32_t error_code)
{
    kprintf(KYEL "No handler for interrupt: %d\n" KCYN, interrupt);
}

void interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
{
    uint32_t error_code;
    asm volatile("movl %%eax, %0\n" : "=r"(error_code) : : "ebx", "eax");

    kernel_page();
    if (interrupt_callbacks[interrupt] != nullptr) {
        scheduler_save_current_thread(frame);
        interrupt_callbacks[interrupt](interrupt, error_code);
    }

    scheduler_switch_current_thread_page();
    outb(0x20, 0x20);
}

void idt_set(const int interrupt, const INTERRUPT_HANDLER_FUNCTION handler)
{
    struct idt_desc *desc = &idt_descriptors[interrupt];
    desc->offset_1        = (uint32_t)handler & 0x0000FFFF;
    desc->selector        = KERNEL_CODE_SELECTOR;
    desc->zero            = 0;

    // This value configures various attributes of the interrupt descriptor, such as the type,
    // privilege level, and whether the descriptor is present.  In this context:
    // The first bit (1) indicates that the descriptor is present.
    // The next two bits (11) set the privilege level (DPL) to 3
    // The next bit (1) is always set to 1.
    // The last four bits (1110) specify the type of gate (e.g., 32-bit interrupt gate).
    desc->type_attr = 0b11101110; // 0xEE

    desc->offset_2 = (uint32_t)handler >> 16;
}

void idt_exception_handler(int interrupt, uint32_t error_code)
{
    if (interrupt == 14) {
        uint32_t faulting_address;
        asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
        kprintf(KRED "\nFaulting address:" KWHT " %x\n", faulting_address);
        auto symbol = debug_symbol_lookup(faulting_address);
        if (symbol.name) {
            kprintf(KRED "Symbol:" KWHT " %s (%x)\n", symbol.name, symbol.address);
        }

        kprintf(KYEL "Error code:" KWHT " %x\n", error_code);
        kprintf(KYEL "P:" KWHT " %x\n", error_code & PAGE_FAULT_PRESENT_MASK ? 1 : 0);
        kprintf(KYEL "W:" KWHT " %x\n", error_code & PAGE_FAULT_WRITE_MASK ? 1 : 0);
        kprintf(KYEL "U:" KWHT " %x\n", error_code & PAGE_FAULT_USER_MASK ? 1 : 0);
        kprintf(KYEL "R:" KWHT " %x\n", error_code & PAGE_FAULT_RESERVED_MASK ? 1 : 0);
        kprintf(KYEL "I:" KWHT " %x\n", error_code & PAGE_FAULT_ID_MASK ? 1 : 0);
        kprintf(KYEL "PK:" KWHT " %x\n", error_code & PAGE_FAULT_PK_MASK ? 1 : 0);
        kprintf(KYEL "SS:" KWHT " %x\n", error_code & PAGE_FAULT_SS_MASK ? 1 : 0);
    } else if (EXCEPTION_HAS_ERROR_CODE(interrupt)) {
        kprintf(KYEL "Error code:" KWHT " %x\n", error_code);
    }

    kprintf(KRED "Exception:" KWHT " %x " KRED "%s\n" KWHT, interrupt, exception_messages[interrupt]);

    int pid = scheduler_get_current_thread()->process->pid;
    char name[MAX_PATH_LENGTH];
    strncpy(name, scheduler_get_current_thread()->process->file_name, sizeof(name));
    process_zombify(scheduler_get_current_thread()->process);
    kprintf("\nThe process %s (%d) has been terminated.", name, pid);
    schedule();
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));

    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base  = (uintptr_t)idt_descriptors;

    for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
        idt_set(i, interrupt_pointer_table[i]);
    }

    for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
        idt_register_interrupt_callback(i, no_interrupt_handler);
    }

    for (int i = 0; i < 0x20; i++) {
        idt_register_interrupt_callback(i, idt_exception_handler);
    }

    // idt_register_interrupt_callback(0x20, idt_clock);
    idt_set(0x80, isr80h_wrapper);

    idt_load(&idtr_descriptor);
}

int idt_register_interrupt_callback(const int interrupt, const INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    if (interrupt < 0 || interrupt >= TOTAL_INTERRUPTS) {
        dbgprintf("Interrupt out of bounds: %d\n", interrupt);
        ASSERT(false, "Interrupt out of bounds");
        return -EINVARG;
    }
    dbgprintf("Registering interrupt callback: %d\n", interrupt);

    interrupt_callbacks[interrupt] = interrupt_callback;
    return ALL_OK;
}

void register_syscall(const int command, const SYSCALL_HANDLER_FUNCTION handler)
{
    if (command < 0 || command >= MAX_SYSCALLS) {
        panic("The command is out of bounds");
    }

    if (syscalls[command]) {
        kprintf("The syscall %x is already registered\n", syscalls[command]);
        panic("The syscall is already registered");
    }

    syscalls[command] = handler;
}

void *handle_syscall(const int syscall, struct interrupt_frame *frame)
{
    if (syscall < 0 || syscall >= MAX_SYSCALLS) {
        warningf("Invalid command: %d\n", syscall);
        kprintf("Invalid command: %d\n", syscall);
        ASSERT(false, "Invalid command");
        return NULL;
    }

    const SYSCALL_HANDLER_FUNCTION handler = syscalls[syscall];
    if (!handler) {
        warningf("No handler for command: %d\n", syscall);
        ASSERT(false, "No handler for command");
        return NULL;
    }

    return handler(frame);
}

void *syscall_handler(const int syscalll, struct interrupt_frame *frame)
{
    kernel_page();
    scheduler_save_current_thread(frame);

    void *res = handle_syscall(syscalll, frame);

    scheduler_switch_current_thread_page();

    return res;
}