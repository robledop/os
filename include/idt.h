#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include "types.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table

enum interrupt_type {
    // 32-bit interrupt gate
    // Interrupt gates are used for hardware interrupts. They clear the IF flag, so the processor will not be interrupted
    interrupt_gate,
    // 32-bit trap gate
    // Trap gates are used for exceptions. They do not clear the IF flag, so the processor can be interrupted
    trap_gate,
};

struct idt_desc {
    uint16_t offset_1; // offset bits 0..15
    uint16_t selector; // a code segment selector in GDT or LDT
    uint8_t zero;      // unused, set to 0
    uint8_t type_attr; // type and attributes, see below
    uint16_t offset_2; // offset bits 16..31
} __attribute__((packed));

struct idtr_desc {
    uint16_t limit; // size of idt
    uint32_t base;  // base address of idt
} __attribute__((packed));

struct interrupt_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

void idt_init();
// void enable_interrupts();
// void disable_interrupts();

typedef void *(*SYSCALL_HANDLER_FUNCTION)(struct interrupt_frame *frame);
typedef void (*INTERRUPT_CALLBACK_FUNCTION)(int interrupt, const struct interrupt_frame *frame);

void register_syscall(int syscall, SYSCALL_HANDLER_FUNCTION handler);
int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback);

/*
 * Page fault error code
 *  31              15                             4               0
 *  +---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
 *  |   Reserved   | SGX |   Reserved   | SS | PK | I | R | U | W | P |
 *  +---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
 */

// Page fault error code masks
// https://wiki.osdev.org/Exceptions

// P: 0 = The fault was caused by a non-present page.
// P: 1 = The fault was caused by a protection violation.
#define PAGE_FAULT_PRESENT_MASK 0x1
// W/R: 0 = The access that caused the fault was a read.
// W/R: 1 = The access that caused the fault was a write.
#define PAGE_FAULT_WRITE_MASK 0x2
// U/S: 0 = The access that caused the fault was supervisor mode.
// U/S: 1 = The access that caused the fault was user mode.
#define PAGE_FAULT_USER_MASK 0x4
// R: 0 = The fault was caused by a reserved bit set to 1 in a page directory entry or page table entry.
// R: 1 = The fault was caused by a reserved bit set to 1 in a page directory entry, page table entry, or descriptor.
#define PAGE_FAULT_RESERVED_MASK 0x8
// I: 0 = The fault was not caused by an instruction fetch.
// I: 1 = The fault was caused by an instruction fetch.
#define PAGE_FAULT_ID_MASK 0x10
// PK: 0 = The fault was not caused by protection keys.
// PK: 1 = The fault was caused by protection keys.
#define PAGE_FAULT_PK_MASK 0x20

#define PAGE_FAULT_SS_MASK 0x4000
// SS: 0 = The fault was not caused by SGX.
// SS: 1 = The fault was caused by SGX.
#define PAGE_FAULT_SGX_MASK 0x4000

/// @brief Checks if the interrupt has an error code
#define EXCEPTION_HAS_ERROR_CODE(interrupt)                                                                            \
    ((interrupt == 8) || (interrupt == 10) || (interrupt == 11) || (interrupt == 12) || (interrupt == 13) ||           \
     (interrupt == 14) || (interrupt == 17) || (interrupt == 18) || (interrupt == 19))
