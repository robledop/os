#include "idt.h"
#include <pic.h>
#include <scheduler.h>
#include <string.h>
#include <x86.h>
#include "config.h"
#include "debug.h"
#include "kernel.h"
#include "memory.h"
#include "serial.h"
#include "status.h"
#include "vga_buffer.h"

extern struct thread *current_thread;

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

void no_interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
{
    kprintf(KYEL "No handler for interrupt: %d\n" KCYN, interrupt);
}

void interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
{
    if (interrupt_callbacks[interrupt] != nullptr) {
        kernel_page();
        scheduler_save_current_thread(frame);
        interrupt_callbacks[interrupt](interrupt, frame);
        scheduler_switch_current_thread_page();
    }

    // External interrupts are special.
    //   We only handle one at a time (so interrupts must be off)
    //   and they need to be acknowledged on the PIC (see below).
    //   An external interrupt handler cannot sleep.
    if (interrupt >= 0x20 && interrupt < 0x30) {
        pic_acknowledge(interrupt);
    }
}

void idt_set(const int interrupt, const INTERRUPT_HANDLER_FUNCTION handler, const enum interrupt_type type)
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
    // (1111) specifies a 32-bit trap gate.
    switch (type) {
    case interrupt_gate:
        desc->type_attr = 0b11101110; // 0xEE
        break;
    case trap_gate:
        desc->type_attr = 0b11101111; // 0xEF
        break;
    default:
        panic("Unsupported interrupt type");
    }
    // desc->type_attr = 0b11101110; // 0xEE

    desc->offset_2 = (uint32_t)handler >> 16;
}

void idt_exception_handler(int interrupt, const struct interrupt_frame *frame)
{
    // Page fault exception
    if (interrupt == 14) {
        const uint32_t faulting_address = read_cr2();
        kprintf(KYEL "\nFaulting address:" KWHT " %x\n", faulting_address);

        // If the faulting address is not in the user space, try to find the closest function symbol
        if (!(frame->eax & PAGE_FAULT_USER_MASK)) {
            auto const symbol = debug_function_symbol_lookup(faulting_address);
            if (symbol.name) {
                kprintf(KYEL "Closest function symbol:" KWHT " %s (%x)\n", symbol.name, symbol.address);
            }
        }

        kprintf(KYEL "Error code:" KWHT " %x\n", frame->eax);
        kprintf(KYEL "P:" KWHT " %s ", frame->eax & PAGE_FAULT_PRESENT_MASK ? "true" : "false");
        kprintf(KYEL "W:" KWHT " %s ", frame->eax & PAGE_FAULT_WRITE_MASK ? "true" : "false");
        kprintf(KYEL "U:" KWHT " %s ", frame->eax & PAGE_FAULT_USER_MASK ? "true" : "false");
        kprintf(KYEL "R:" KWHT " %s ", frame->eax & PAGE_FAULT_RESERVED_MASK ? "true" : "false");
        kprintf(KYEL "I:" KWHT " %s ", frame->eax & PAGE_FAULT_ID_MASK ? "true" : "false");
        kprintf(KYEL "PK:" KWHT " %s ", frame->eax & PAGE_FAULT_PK_MASK ? "true" : "false");
        kprintf(KYEL "SS:" KWHT " %s\n", frame->eax & PAGE_FAULT_SS_MASK ? "true" : "false");
    } else if (EXCEPTION_HAS_ERROR_CODE(interrupt)) {
        kprintf(KYEL "Error code:" KWHT " %x\n", frame->eax);
    }

    kprintf(KRED "Exception:" KWHT " %x " KRED "%s\n" KWHT, interrupt, exception_messages[interrupt]);

    if (frame->cs == KERNEL_CODE_SELECTOR) {
        kprintf(KBOLD KWHT "The exception occurred in kernel mode.\n" KRESET);
    } else {
        kprintf(KBOLD KWHT "The exception occurred in user mode.\n" KRESET);
    }

    debug_stats();

    const int pid = scheduler_get_current_process()->pid;
    char name[MAX_PATH_LENGTH];
    strncpy(name, scheduler_get_current_process()->file_name, sizeof(name));
    process_zombify(scheduler_get_current_process());
    kprintf("The process %s (%d) has been terminated.\n", name, pid);

    sti();
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));

    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base  = (uintptr_t)idt_descriptors;

    // Exceptions
    for (int i = 0; i < 0x20; i++) {
        idt_set(i, interrupt_pointer_table[i], trap_gate);
    }

    // Page fault exception
    // We want interrupts off for page faults, so we can read the faulting address and error code.
    idt_set(0xE, interrupt_pointer_table[0xE], interrupt_gate);

    // Interrupts
    for (int i = 0x20; i < TOTAL_INTERRUPTS; i++) {
        idt_set(i, interrupt_pointer_table[i], interrupt_gate);
    }

    for (int i = 0; i < TOTAL_INTERRUPTS; i++) {
        idt_register_interrupt_callback(i, no_interrupt_handler);
    }

    for (int i = 0; i < 0x20; i++) {
        idt_register_interrupt_callback(i, idt_exception_handler);
    }

    idt_set(0x80, isr80h_wrapper, interrupt_gate);

    idt_load(&idtr_descriptor);
}

int idt_register_interrupt_callback(const int interrupt, const INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    ASSERT(interrupt >= 0 && interrupt < TOTAL_INTERRUPTS, "Interrupt out of bounds");

    dbgprintf("Registering interrupt callback: %d\n", interrupt);

    interrupt_callbacks[interrupt] = interrupt_callback;
    return ALL_OK;
}

void register_syscall(const int syscall, const SYSCALL_HANDLER_FUNCTION handler)
{
    ASSERT(syscall >= 0 && syscall < MAX_SYSCALLS, "Invalid syscall");
    ASSERT(!syscalls[syscall], "The syscall is already registered");

    syscalls[syscall] = handler;
}

void *handle_syscall(const int syscall, struct interrupt_frame *frame)
{
    ASSERT(syscall >= 0 && syscall < MAX_SYSCALLS, "Invalid syscall");
    auto const handler = syscalls[syscall];
    ASSERT(handler);

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