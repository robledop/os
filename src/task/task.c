#include "task.h"
#include "status.h"
#include "kernel.h"
#include "serial.h"
#include "kheap.h"
#include "memory.h"
#include "idt.h"

struct task *current_task = 0;
struct task *task_tail = 0;
struct task *task_head = 0;
int task_init(struct task *task, struct process *process);

struct task *task_current()
{
    return current_task;
}

struct task *task_get_next()
{
    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task *task)
{
    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next();
    }
}

int task_free(struct task *task)
{
    paging_free_directory(task->page_directory);
    task_list_remove(task);

    kfree(task);

    return ALL_OK;
}

struct task *task_create(struct process *process)
{
    dbgprintf("Creating task for process %x\n", process);

    int res = 0;

    struct task *task = (struct task *)kmalloc(sizeof(struct task));
    if (!task)
    {
        dbgprintf("Failed to allocate memory for task\n");
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != ALL_OK)
    {
        dbgprintf("Failed to initialize task\n");
        goto out;
    }

    if (!task_head)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
    }
    else
    {
        task_tail->next = task;
        task->prev = task_tail;
        task_tail = task;
    }

out:
    if (ISERR(res))
    {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

int task_switch(struct task *task)
{
    dbgprintf("Switching to task %x from process %d\n", task, task->process->pid);

    current_task = task;
    paging_switch_directory(task->page_directory);
    return ALL_OK;
}

void task_save_state(struct task *task, struct interrupt_frame *frame)
{
    task->registers.edi = frame->edi;
    task->registers.esi = frame->esi;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.edx = frame->edx;
    task->registers.ecx = frame->ecx;
    task->registers.eax = frame->eax;

    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
}

void task_current_save_state(struct interrupt_frame *frame)
{
    struct task *task = task_current();
    if (!task)
    {
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

void task_run_first_ever_task()
{
    if (!current_task)
    {
        panic("No tasks to run");
    }

    dbgprintf("Running first ever task %x\n", current_task);

    task_switch(task_head);
    task_return(&task_head->registers);
}

int task_init(struct task *task, struct process *process)
{
    memset(task, 0, sizeof(struct task));
    task->page_directory = paging_create_directory(
        PAGING_DIRECTORY_ENTRY_IS_PRESENT |
        PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    if (!task->page_directory)
    {
        dbgprintf("Failed to create page directory for task %x\n", &task);
        return -ENOMEM;
    }

    task->registers.ip = PROGRAM_VIRTUAL_ADDRESS;
    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.esp = PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    dbgprintf("Task %x initialized\n", task);
    dbgprintf("Task %x page directory %x\n", task, task->page_directory->directory_entry);
    dbgprintf("Task %x registers ip %x\n", task, task->registers.ip);

    return ALL_OK;
}
