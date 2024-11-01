#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <idt.h>
#include <list.h>
#include "paging.h"
#include "process.h"

typedef struct cpu_state {
    // General-purpose registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // Instruction pointer and flags
    uint32_t eip, eflags;
    // Segment registers (optional for kernel mode if flat segmentation is used)
    uint32_t cs;
} cpu_state_t;

struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
};


struct thread {
    struct registers registers;
    // struct registers kernel_state;
    struct process *process;
    // struct list_elem allelem;           /**< List element for all threads list. */
    struct list_elem elem;
    // struct thread *next;
    // struct thread *prev;
    int tty;
};

struct thread *thread_create(struct process *process);
int thread_free(struct thread *thread);
void restore_general_purpose_registers(struct registers *registers);
int copy_string_from_thread(const struct thread *thread, const void *virtual, void *physical, int max);
void *thread_peek_stack_item(const struct thread *task, int index);
void *thread_virtual_to_physical_address(const struct thread *task, void *virtual_address);
int thread_page_thread(const struct thread *thread);
void thread_save_state(struct thread *thread, const struct interrupt_frame *frame);
void thread_copy_registers(struct thread *dest, const struct thread *src);
struct registers interrupt_frame_to_registers(const struct interrupt_frame *frame);
struct interrupt_frame registers_to_interrupt_frame(const struct registers *registers);
void thread_switch(struct registers *registers);
