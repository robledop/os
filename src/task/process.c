#include "process.h"
#include "memory.h"
#include "status.h"
#include "serial.h"
#include "task.h"
#include "kheap.h"
#include "file.h"
#include "string.h"
#include "kernel.h"
#include "paging.h"
#include "terminal.h"

struct process *current_process = 0;

static struct process *processes[MAX_PROCESSES] = {};

static void process_init(struct process *process)
{
    memset(process, 0, sizeof(struct process));
}

struct process *process_current()
{
    return current_process;
}

struct process *process_get(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
    {
        dbgprintf("Invalid process id: %d\n", pid);
        return NULL;
    }

    return processes[pid];
}

static int process_load_binary(const char *file_name, struct process *process)
{
    dbgprintf("Loading binary %s\n", file_name);

    int res = 0;
    int fd = fopen(file_name, "r");
    if (!fd)
    {
        dbgprintf("Failed to open file %s\n", file_name);
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != ALL_OK)
    {
        dbgprintf("Failed to get file stat for %s\n", file_name);
        res = -EIO;
        goto out;
    }

    void *program = kzalloc(stat.size);
    if (!program)
    {
        dbgprintf("Failed to allocate memory for program %s\n", file_name);
        res = -ENOMEM;
        goto out;
    }

    if (fread(program, stat.size, 1, fd) != 1)
    {
        dbgprintf("Failed to read program %s\n", file_name);
        res = -EIO;
        goto out;
    }

    process->pointer = program;
    process->size = stat.size;

    dbgprintf("Loaded binary %s to %x\n", file_name, program);
    dbgprintf("Program size: %d\n", stat.size);

out:
    fclose(fd);
    return res;
}

static int process_load_data(const char *file_name, struct process *process)
{
    int res = 0;
    res = process_load_binary(file_name, process);

    return res;
}

int process_map_binary(struct process *process)
{
    int res = 0;
    paging_map_to(
        process->task->page_directory,
        (void *)PROGRAM_VIRTUAL_ADDRESS,
        process->pointer,
        paging_align_address(process->pointer + process->size),
        PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_SUPERVISOR);

    return res;
}

int process_map_memory(struct process *process)
{
    int res = 0;
    res = process_map_binary(process);

    return res;
}

int process_get_free_slot()
{
    for (size_t i = 0; i < MAX_PROCESSES; i++)
    {
        if (processes[i] == 0)
        {
            return i;
        }
    }

    return -EINSTKN;
}

int process_load(const char *file_name, struct process **process)
{
    dbgprintf("Loading process %s\n", file_name);
    int res = 0;
    int process_slot = process_get_free_slot();
    if (process_slot < 0)
    {
        dbgprintf("Failed to get free slot for process %s\n", file_name);
        res = -EINSTKN;
        goto out;
    }

    dbgprintf("Process %s will be loaded to slot %d\n", file_name, process_slot);

    res = process_load_for_slot(file_name, process, process_slot);

    kprint(KMAG"Process %s (pid: %d) is now running\n", (*process)->file_name, (*process)->pid);

out:
    return res;
}

int process_load_for_slot(const char *file_name, struct process **process, uint16_t slot)
{
    int res = 0;
    struct task *task = 0;
    struct process *proc;
    void *program_stack_pointer = 0;

    dbgprintf("Loading process %s to slot %d\n", file_name, slot);

    if (process_get(slot) != 0)
    {
        dbgprintf("Slot %d is already taken\n", slot);
        res = -EINSTKN;
        goto out;
    }
    proc = kzalloc(sizeof(struct process));
    if (!proc)
    {
        dbgprintf("Failed to allocate memory for process %d\n", slot);
        res = -ENOMEM;
        goto out;
    }

    process_init(proc);
    res = process_load_data(file_name, proc);
    if (res < 0)
    {
        dbgprintf("Failed to load process %s\n", file_name);
        goto out;
    }

    program_stack_pointer = kzalloc(USER_PROGRAM_STACK_SIZE);
    if (!program_stack_pointer)
    {
        dbgprintf("Failed to allocate memory for program stack %s\n", file_name);
        res = -ENOMEM;
        goto out;
    }

    strncpy(proc->file_name, file_name, sizeof(proc->file_name));
    proc->stack = program_stack_pointer;
    proc->pid = slot;

    dbgprintf("Process %s stack pointer is %x and process id is %d\n", file_name, program_stack_pointer, slot);

    task = task_create(proc);
    if (ERROR_I(task) == 0)
    {
        dbgprintf("Failed to create task for process %s\n", file_name);
        res = -ENOMEM;
        goto out;
    }

    proc->task = task;

    res = process_map_memory(proc);
    if (res < 0)
    {
        dbgprintf("Failed to map memory for process %s\n", file_name);
        goto out;
    }

    *process = proc;

    processes[slot] = proc;

out:
    if (ISERR(res))
    {
        if (proc && proc->task)
        {
            task_free(proc->task);
        }

        // TODO: free process data
    }
    return res;
}