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

struct idt_desc idt_descriptors[TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void *interrupt_pointer_table[TOTAL_INTERRUPTS];
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[TOTAL_INTERRUPTS];
static ISR80H_COMMAND isr80h_commands[MAX_ISR80H_COMMANDS];
extern void idt_load(struct idtr_desc *ptr);
extern void no_interrupt();
extern void isr80h_wrapper();

void no_interrupt_handler() { outb(0x20, 0x20); }

void interrupt_handler(int interrupt, struct interrupt_frame *frame)
{
    if (interrupt == 32)
    {
        outb(0x20, 0x20);
        return;
    }

    kernel_page();
    if (interrupt_callbacks[interrupt] != 0)
    {
        task_current_save_state(frame);
        interrupt_callbacks[interrupt]();
    }

    task_page();
    outb(0x20, 0x20);
}

// Interrupt 0
// This function is called when a division by zero occurs.
// We simply print an error message and halt the CPU.
void idt_zero() { panic("Division by zero error"); }

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

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));

    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uintptr_t)idt_descriptors;

    for (size_t i = 0; i < TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, interrupt_pointer_table[i]);
    }

    idt_set(0, idt_zero);
    idt_set(0x80, isr80h_wrapper);

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

void isr80h_register_command(int command, ISR80H_COMMAND handler)
{
    if (command < 0 || command >= MAX_ISR80H_COMMANDS)
    {
        dbgprintf("Command out of bounds: %d\n", command);
        panic("The command is out of bounds");
    }

    if (isr80h_commands[command])
    {
        dbgprintf("Command already registered: %d\n", command);
        panic("The command is already registered");
    }

    dbgprintf("Registering command: %d\n", command);

    isr80h_commands[command] = handler;
}

void *isr80h_handle_command(int command, struct interrupt_frame *frame)
{
    void *result = 0;

    if (command < 0 || command >= MAX_ISR80H_COMMANDS)
    {
        dbgprintf("Invalid command: %d\n", command);
        return NULL;
    }

    ISR80H_COMMAND handler = isr80h_commands[command];
    if (!handler)
    {
        dbgprintf("No handler for command: %d\n", command);
        return NULL;
    }

    result = handler(frame);

    return result;
}

void *isr80h_handler(int command, struct interrupt_frame *frame)
{
    void *res = 0;
    kernel_page();
    task_current_save_state(frame);
    res = isr80h_handle_command(command, frame);
    task_page();

    return res;
}