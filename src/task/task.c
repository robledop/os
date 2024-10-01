#include "task.h"
#include "status.h"
#include "kernel.h"
#include "serial.h"
#include "kheap.h"
#include "memory.h"

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
    task->registers.esp = PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return ALL_OK;
}
