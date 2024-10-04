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

void idt_exception_handler()
{
    process_terminate(task_current()->process);
    kprintf("The process with id %d has been terminated.\n", task_current()->process->pid);
    task_next();
}

void idt_clock()
{
    outb(0x20, 0x20);
    // task_next();
}

void idt_debug_exception()
{
    kprintf(KRED "Debug exception\n");
    idt_exception_handler();
}

void idt_nmi()
{
    kprintf(KRED "NMI\n");
    idt_exception_handler();
}

void idt_breakpoint()
{
    kprintf(KRED "Breakpoint\n");
    idt_exception_handler();
}

void idt_overflow()
{
    kprintf(KRED "Overflow\n");
    idt_exception_handler();
}

void idt_bound_range_exceeded()
{
    kprintf(KRED "Bound range exceeded\n");
    idt_exception_handler();
}

void idt_invalid_opcode()
{
    kprintf(KRED "Invalid opcode\n");
    idt_exception_handler();
}

void idt_device_not_available()
{
    kprintf(KRED "Device not available\n");
    idt_exception_handler();
}

void idt_double_fault()
{
    kprintf(KRED "Double fault\n");
    idt_exception_handler();
}

void idt_coprocessor_segment_overrun()
{
    kprintf(KRED "Coprocessor segment overrun\n");
    idt_exception_handler();
}

void idt_invalid_tss()
{
    kprintf(KRED "Invalid TSS\n");
    idt_exception_handler();
}

void idt_segment_not_present()
{
    kprintf(KRED "Segment not present\n");
    idt_exception_handler();
}

void idt_stack_segment_fault()
{
    kprintf(KRED "Stack segment fault\n");
    idt_exception_handler();
}

void idt_general_protection()
{
    kprintf(KRED "General protection\n");
    idt_exception_handler();
}

void idt_page_fault()
{
    kprintf(KRED "Page fault\n");
    idt_exception_handler();
}

void idt_x87_fpu_error()
{
    kprintf(KRED "x87 FPU error\n");
    idt_exception_handler();
}

void idt_alignment_check()
{
    kprintf(KRED "Alignment check\n");
    idt_exception_handler();
}

void idt_machine_check()
{
    kprintf(KRED "Machine check\n");
    idt_exception_handler();
}

void idt_simd_fpu_exception()
{
    kprintf(KRED "SIMD FPU exception\n");
    idt_exception_handler();
}

void idt_virtualization_exception()
{
    kprintf(KRED "Virtualization exception\n");
    idt_exception_handler();
}

void idt_security_exception()
{
    kprintf(KRED "Security exception\n");
    idt_exception_handler();
}

void idt_interrupt_request()
{
    kprintf(KRED "Interrupt request\n");
    idt_exception_handler();
}

void idt_fpu_error()
{
    kprintf(KRED "FPU error\n");
    idt_exception_handler();
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

    idt_register_interrupt_callback(0, idt_zero);
    idt_register_interrupt_callback(1, idt_debug_exception);
    idt_register_interrupt_callback(2, idt_nmi);
    idt_register_interrupt_callback(3, idt_breakpoint);
    idt_register_interrupt_callback(4, idt_overflow);
    idt_register_interrupt_callback(5, idt_bound_range_exceeded);
    idt_register_interrupt_callback(6, idt_invalid_opcode);
    idt_register_interrupt_callback(7, idt_device_not_available);
    idt_register_interrupt_callback(8, idt_double_fault);
    idt_register_interrupt_callback(9, idt_coprocessor_segment_overrun);
    idt_register_interrupt_callback(10, idt_invalid_tss);
    idt_register_interrupt_callback(11, idt_segment_not_present);
    idt_register_interrupt_callback(12, idt_stack_segment_fault);
    idt_register_interrupt_callback(13, idt_general_protection);
    idt_register_interrupt_callback(14, idt_page_fault);
    idt_register_interrupt_callback(16, idt_x87_fpu_error);
    idt_register_interrupt_callback(17, idt_alignment_check);
    idt_register_interrupt_callback(18, idt_machine_check);
    idt_register_interrupt_callback(19, idt_simd_fpu_exception);
    idt_register_interrupt_callback(20, idt_virtualization_exception);
    idt_register_interrupt_callback(30, idt_security_exception);

    // for (int i = 0; i < 0x20; i++)
    // {
    //     idt_register_interrupt_callback(i, idt_exception_handler);
    // }

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

void isr80h_register_command(int command, ISR80H_COMMAND handler)
{
    if (command < 0 || command >= MAX_ISR80H_COMMANDS)
    {
        panic("The command is out of bounds");
    }

    if (isr80h_commands[command])
    {
        panic("The command is already registered");
    }

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