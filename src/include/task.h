#pragma once

#include "paging.h"
#include "process.h"

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

struct process;
struct interrupt_frame;

struct task {
    struct page_directory *page_directory;
    struct registers registers;
    struct process *process;
    struct task *next;
    struct task *prev;
    int tty;
};

struct task *task_create(struct process *process);
int task_free(struct task *task);
void restore_general_purpose_registers(struct registers *registers);
int copy_string_from_task(const struct task *task, const void *virtual, void *physical, int max);
void *task_peek_stack_item(const struct task *task, int index);
void *task_virtual_to_physical_address(const struct task *task, void *virtual_address);
int task_page_task(const struct task *task);
void task_save_state(struct task *task, const struct interrupt_frame *frame);
void task_copy_registers(struct task* dest, const struct task* src);
