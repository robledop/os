#pragma once

#include "types.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table

struct idt_desc
{
    uint16_t offset_1; // offset bits 0..15
    uint16_t selector; // a code segment selector in GDT or LDT
    uint8_t zero;      // unused, set to 0
    uint8_t type_attr; // type and attributes, see below
    uint16_t offset_2; // offset bits 16..31
} __attribute__((packed));

struct idtr_desc
{
    uint16_t limit; // size of idt
    uint32_t base;  // base address of idt
} __attribute__((packed));

struct interrupt_frame
{
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
void enable_interrupts();
void disable_interrupts();

typedef void *(*SYSCALL_HANDLER_FUNCTION)(struct interrupt_frame *frame);
typedef void (*INTERRUPT_CALLBACK_FUNCTION)(int interrupt);

void register_syscall(int command, SYSCALL_HANDLER_FUNCTION handler);
int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION callback);
