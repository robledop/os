#include "task.h"
#include "elfloader.h"
#include "idt.h"
#include "kernel.h"
#include "kernel_heap.h"
#include "memory.h"
#include "process.h"
#include "serial.h"
#include "status.h"
#include "string.h"
#include "vga_buffer.h"

struct task *current_task = nullptr;
struct task *task_tail    = nullptr;
struct task *task_head    = nullptr;
int task_init(struct task *task, struct process *process);

struct task *task_current()
{
    return current_task;
}

struct task *task_get_next()
{
    if (!current_task && !task_head) {
        return nullptr;
    }

    if (!current_task) {
        return task_head;
    }

    if (!current_task->next) {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(const struct task *task)
{
    if (task->prev) {
        task->prev->next = task->next;
    }

    if (task == task_head) {
        task_head = task->next;
    }

    if (task == task_tail) {
        task_tail = task->prev;
    }

    if (task == current_task) {
        current_task = task_get_next();
    }
}

int task_free(struct task *task)
{
    if (task == NULL) {
        return -EINVARG;
    }

    paging_free_directory(task->page_directory);
    task_list_remove(task);

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

    if (!task_head) {
        task_head    = task;
        task_tail    = task;
        current_task = task;
    } else {
        task_tail->next = task;
        task->prev      = task_tail;
        task_tail       = task;
    }

out:
    if (ISERR(res)) {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

int task_switch(struct task *task)
{
    // dbgprintf("Switching to task %x from process %d\n", task, task->process->pid);
    // kprintf("Switching to task %x from process %d\n", task, task->process->pid);

    current_task = task;
    paging_switch_directory(task->page_directory);
    return ALL_OK;
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

void task_current_save_state(const struct interrupt_frame *frame)
{
    struct task *task = task_current();
    if (!task) {
        panic("No current task");
    }

    task_save_state(task, frame);
}

int task_page()
{
    user_registers();
    task_switch(current_task);

    return ALL_OK;
}

int task_page_task(const struct task *task)
{
    user_registers();
    paging_switch_directory(task->page_directory);

    return ALL_OK;
}

void task_run_first_task()
{
    if (!current_task) {
        panic("No tasks to run");
    }

    dbgprintf("Running first ever task %x\n", current_task);

    task_switch(task_head);
    task_return(&task_head->registers);
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
    task->registers.esp = PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    dbgprintf("Task %x initialized\n", task);
    dbgprintf("Task %x page directory %x\n", task, task->page_directory->directory_entry);
    dbgprintf("Task %x registers ip %x\n", task, task->registers.ip);

    return ALL_OK;
}

void *task_get_stack_item(const struct task *task, const int index)
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

void task_next()
{
    struct task *next = task_get_next();
    if (!next) {
        kprintf("\nRestarting the shell");
        start_shell(0);
        return;
    }

    auto const process = next->process;

    // TODO: Add a way for the child process to signal to the parent that it has exited
    if (process->state == WAITING) {
        auto const child = find_child_process_by_pid(process, process->wait_pid);
        if (child->state == ZOMBIE) {
            process->state     = RUNNING;
            process->exit_code = child->exit_code;
            process->wait_pid  = 0;
            remove_child(process, child);
            process_unlink(child);
        } else {
            task_switch(child->task);
            task_return(&child->task->registers);
        }
    }

    // if (process->priority == 0) {
    //     task_switch(process->task);
    //     return;
    // }

    task_switch(next);
    task_return(&next->registers);
}