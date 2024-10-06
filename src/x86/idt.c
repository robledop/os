#include "idt.h"
#include "config.h"
#include "io.h"
#include "kernel.h"
#include "memory.h"
#include "serial.h"
#include "status.h"
#include "task.h"
#include "terminal.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table

char *exception_messages[] = {
    "Division By Zero",
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

extern void *interrupt_pointer_table[TOTAL_INTERRUPTS];
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[TOTAL_INTERRUPTS];
static SYSCALL_HANDLER_FUNCTION syscalls[MAX_SYSCALLS];
extern void idt_load(struct idtr_desc *ptr);
extern void isr80h_wrapper();

void interrupt_handler(int interrupt, struct interrupt_frame *frame)
{
    kernel_page();
    if (interrupt_callbacks[interrupt] != 0)
    {
        task_current_save_state(frame);
        interrupt_callbacks[interrupt](interrupt);
    }

    task_page();
    outb(0x20, 0x20);
}

void idt_set(int interrupt, void *handler)
{
    struct idt_desc *desc = &idt_descriptors[interrupt];
    desc->offset_1 = (uint32_t)handler & 0x0000FFFF;
    desc->selector = CODE_SELECTOR;
    desc->zero = 0;

    // This value configures various attributes of the interrupt descriptor, such as the type,
    // privilege level, and whether the descriptor is present.  In this context:
    // The first bit (1) indicates that the descriptor is present.
    // The next two bits (11) set the privilege level (DPL) to 3
    // The next bit (1) is always set to 1.
    // The last four bits (1110) specify the type of gate (e.g., 32-bit interrupt gate).
    desc->type_attr = 0b11101110; // 0xEE

    desc->offset_2 = (uint32_t)handler >> 16;
}

void idt_exception_handler(int interrupt)
{
    kprintf(KRED "\n%s\n" KWHT, exception_messages[interrupt]);
    process_terminate(task_current()->process);
    kprintf("\nThe process with id %d has been terminated.", task_current()->process->pid);
    task_next();
}

void idt_clock(int interrupt)
{
    outb(0x20, 0x20);
    // Uncomment to enable multitasking
    // task_next();
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));

    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uintptr_t)idt_descriptors;

    for (size_t i = 0; i < TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, interrupt_pointer_table[i]);
    }

    idt_set(0x80, isr80h_wrapper);

    for (int i = 0; i < 0x20; i++)
    {
        idt_register_interrupt_callback(i, idt_exception_handler);
    }

    idt_register_interrupt_callback(0x20, idt_clock);

    idt_load(&idtr_descriptor);
}

int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    if (interrupt < 0 || interrupt >= TOTAL_INTERRUPTS)
    {
        dbgprintf("Interrupt out of bounds: %d\n", interrupt);
        return -EINVARG;
    }
    dbgprintf("Registering interrupt callback: %d\n", interrupt);

    interrupt_callbacks[interrupt] = interrupt_callback;
    return ALL_OK;
}

void register_syscall(int command, SYSCALL_HANDLER_FUNCTION handler)
{
    if (command < 0 || command >= MAX_SYSCALLS)
    {
        panic("The command is out of bounds");
    }

    if (syscalls[command])
    {
        kprintf("The syscall is already registered %x\n", syscalls[command]);
        panic("The syscall is already registered");
    }

    syscalls[command] = handler;
}

void *handle_syscall(int syscall, struct interrupt_frame *frame)
{
    void *result = 0;

    if (syscall < 0 || syscall >= MAX_SYSCALLS)
    {
        dbgprintf("Invalid command: %d\n", syscall);
        return NULL;
    }

    SYSCALL_HANDLER_FUNCTION handler = syscalls[syscall];
    if (!handler)
    {
        dbgprintf("No handler for command: %d\n", syscall);
        return NULL;
    }

    result = handler(frame);

    return result;
}

void *syscall_handler(int syscalll, struct interrupt_frame *frame)
{
    void *res = 0;
    kernel_page();
    task_current_save_state(frame);
    res = handle_syscall(syscalll, frame);
    task_page();

    return res;
}