#include "task.h"

#include <scheduler.h>

#include "elfloader.h"
#include "idt.h"
#include "kernel.h"
#include "kernel_heap.h"
#include "memory.h"
#include "process.h"
#include "serial.h"
#include "status.h"
#include "string.h"

int task_init(struct task *task, struct process *process);

int task_free(struct task *task)
{
    if (task == NULL) {
        return -EINVARG;
    }

    paging_free_directory(task->page_directory);
    scheduler_unqueue_task(task);

    kfree(task);

    return ALL_OK;
}

struct task *task_create(struct process *process)
{
    dbgprintf("Creating task for process %x\n", process);

    int res = 0;

    auto task = (struct task *)kmalloc(sizeof(struct task));
    if (!task) {
        dbgprintf("Failed to allocate memory for task\n");
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != ALL_OK) {
        dbgprintf("Failed to initialize task\n");
        goto out;
    }

    scheduler_queue_task(task);

out:
    if (ISERR(res)) {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

/// @brief Set the user mode segments and switch to the task's page directory
/// @param task the task to switch to
int task_page_task(const struct task *task)
{
    set_user_mode_segments();
    paging_switch_directory(task->page_directory);

    return ALL_OK;
}

int copy_string_from_task(const struct task *task, const void *virtual, void *physical, const int max)
{
    if (max >= PAGING_PAGE_SIZE) {
        dbgprintf("String too long\n");
        return -EINVARG;
    }

    int res   = 0;
    char *tmp = kzalloc(max);
    if (!tmp) {
        dbgprintf("Failed to allocate memory for string\n");
        res = -ENOMEM;
        goto out;
    }

    const uint32_t old_entry = paging_get(task->page_directory, tmp);

    paging_map(task->page_directory,
               tmp,
               tmp,
               PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
                   PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(task->page_directory);
    strncpy(tmp, virtual, max);
    kernel_page();
    res = paging_set(task->page_directory, tmp, old_entry);
    if (res < 0) {
        dbgprintf("Failed to set page\n");
        res = -EIO;
        goto out_free;
    }

    strncpy(physical, tmp, max);

out_free:
    kfree(tmp);
out:
    return res;
}

int task_init(struct task *task, struct process *process)
{
    memset(task, 0, sizeof(struct task));
    task->page_directory =
        paging_create_directory(PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);

    if (!task->page_directory) {
        dbgprintf("Failed to create page directory for task %x\n", &task);
        return -ENOMEM;
    }

    switch (process->file_type) {
    case PROCESS_FILE_TYPE_BINARY:
        task->registers.ip = PROGRAM_VIRTUAL_ADDRESS;
        break;
    case PROCESS_FILE_TYPE_ELF:
        task->registers.ip = elf_header(process->elf_file)->e_entry;
        break;
    default:
        panic("Unknown process file type");
        break;
    }

    task->registers.ss  = USER_DATA_SELECTOR;
    task->registers.cs  = USER_CODE_SELECTOR;
    task->registers.esp = USER_STACK_TOP;

    task->process = process;

    dbgprintf("Task %x initialized\n", task);
    dbgprintf("Task %x page directory %x\n", task, task->page_directory->directory_entry);
    dbgprintf("Task %x registers ip %x\n", task, task->registers.ip);

    return ALL_OK;
}

void *task_peek_stack_item(const struct task *task, const int index)
{
    const uint32_t *stack_pointer = (uint32_t *)task->registers.esp;
    task_page_task(task);
    auto const result = (void *)stack_pointer[index];
    kernel_page();
    return result;
}

void *task_virtual_to_physical_address(const struct task *task, void *virtual_address)
{
    return paging_get_physical_address(task->page_directory, virtual_address);
}

void task_save_state(struct task *task, const struct interrupt_frame *frame)
{
    task->registers.edi = frame->edi;
    task->registers.esi = frame->esi;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.edx = frame->edx;
    task->registers.ecx = frame->ecx;
    task->registers.eax = frame->eax;

    task->registers.ip    = frame->ip;
    task->registers.cs    = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp   = frame->esp;
    task->registers.ss    = frame->ss;
}

void task_copy_registers(struct task *dest, const struct task *src)
{
    dest->registers.edi = src->registers.edi;
    dest->registers.esi = src->registers.esi;
    dest->registers.ebp = src->registers.ebp;
    dest->registers.ebx = src->registers.ebx;
    dest->registers.edx = src->registers.edx;
    dest->registers.ecx = src->registers.ecx;
    dest->registers.eax = src->registers.eax;

    dest->registers.ip    = src->registers.ip;
    dest->registers.cs    = src->registers.cs;
    dest->registers.flags = src->registers.flags;
    dest->registers.esp   = src->registers.esp;
    dest->registers.ss    = src->registers.ss;
}
