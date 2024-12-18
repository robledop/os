#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <idt.h>
#include <list.h>
#include <paging.h>
#include <process.h>

#define THREAD_MAGIC 0x1eaadf71

struct registers {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
};


struct thread {
    struct registers registers;
    // struct registers kernel_state;
    struct process *process;
    // struct list_elem allelem;           /**< List element for all threads list. */
    struct list_elem elem;
    unsigned magic;
};

__attribute__((nonnull)) struct thread *thread_create(struct process *process);
__attribute__((nonnull)) int thread_free(struct thread *thread);
__attribute__((nonnull)) void restore_general_purpose_registers(struct registers *registers);
__attribute__((nonnull)) int copy_string_from_thread(const struct thread *thread, const void *virtual, void *physical,
                                                     size_t max);
__attribute__((nonnull)) void *thread_peek_stack_item(const struct thread *task, int index);
__attribute__((nonnull)) void *thread_virtual_to_physical_address(const struct thread *task, void *virtual_address);
__attribute__((nonnull)) int thread_page_thread(const struct thread *thread);
__attribute__((nonnull)) void thread_save_state(struct thread *thread, const struct interrupt_frame *frame);
__attribute__((nonnull)) void thread_copy_registers(struct thread *dest, const struct thread *src);
__attribute__((nonnull)) void thread_switch(struct registers *registers);
__attribute__((nonnull)) bool thread_is_valid(const struct thread *thread);
