#include "idt.h"
#include "config.h"
#include "memory.h"
#include "kernel.h"
#include "io.h"
#include "terminal.h"
#include "serial.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table

struct idt_desc idt_descriptors[TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

extern void idt_load(struct idtr_desc *ptr);
extern void int21h();
extern void no_interrupt();

void int21h_handler()
{
    print("Keyboard pressed!\n");
    dbgprintf("Keyboard pressed!\n");   

    outb(0x20, 0x20);
}

void no_interrupt_handler()
{
    outb(0x20, 0x20);
}

// Interrupt 0
// This function is called when a division by zero occurs.
// We simply print an error message and halt the CPU.
void idt_zero()
{
    panic("Division by zero error");
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

void idt_init()
{
    dbgprintf("Initializing interrupt descriptor table\n");
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uintptr_t)idt_descriptors;

    for (size_t i = 0; i < TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, no_interrupt);
    }

    idt_set(0, idt_zero);
    idt_set(0x21, int21h);

    idt_load(&idtr_descriptor);
}