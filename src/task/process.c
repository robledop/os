#include "process.h"
#include "assert.h"
#include "elfloader.h"
#include "file.h"
#include "kernel.h"
#include "kernel_heap.h"
#include "memory.h"
#include "paging.h"
#include "serial.h"
#include "status.h"
#include "string.h"
#include "task.h"
#include "terminal.h"

struct process *current_process = 0;

static struct process *processes[MAX_PROCESSES] = {};

static void process_init(struct process *process) { memset(process, 0, sizeof(struct process)); }

struct process *process_current() {
    return current_process;
}

struct process *process_get(int pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) {
        warningf("Invalid process id: %d\n", pid);
        ASSERT(false, "Invalid process id");
        return NULL;
    }

    return processes[pid];
}

int process_switch(struct process *process) {
    if (!process) {
        return -EINVARG;
    }

    current_process = process;
    return ALL_OK;
}

static int process_find_free_allocation_slot(struct process *process) {
    for (size_t i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == NULL) {
            return i;
        }
    }

    warningf("Failed to find free allocation slot for process %d\n", process->pid);
    ASSERT(false, "Failed to find free allocation slot");
    return -ENOMEM;
}

// static bool process_is_process_pointer(struct process *process, void *ptr)
// {
//     for (size_t i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++)
//     {
//         if (process->allocations[i].ptr == ptr)
//         {
//             return true;
//         }
//     }

//     return false;
// }

static struct process_allocation *process_get_allocation_by_address(struct process *process, void *address) {
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == address) {
            return &process->allocations[i];
        }
    }

    return NULL;
}

int process_terminate_allocations(struct process *process) {
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        // kfree(process->allocations[i].ptr);
        process_free(process, process->allocations[i].ptr);
    }

    return 0;
}

int process_free_program_data(struct process *process) {
    int res = 0;
    switch (process->file_type) {
    case PROCESS_FILE_TYPE_BINARY:
        kfree(process->pointer);
        break;

    case PROCESS_FILE_TYPE_ELF:
        elf_close(process->elf_file);
        break;

    default:
        ASSERT(false, "Unknown process file type");
        res = -EINVARG;
    }
    return res;
}

void process_switch_to_any() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i]) {
            process_switch(processes[i]);
            return;
        }
    }

    // TODO: Restart a shell process

    panic("No processes to switch to.\n");
}

static void process_unlink(struct process *process) {
    processes[process->pid] = 0x00;

    if (current_process == process) {
        process_switch_to_any();
    }
}

int process_terminate(struct process *process) {
    int res = 0;

    res = process_terminate_allocations(process);
    if (res < 0) {
        warningf("Failed to terminate allocations for process %d\n", process->pid);
        ASSERT(false, "Failed to terminate allocations for process");
        return res;
    }

    res = process_free_program_data(process);
    if (res < 0) {
        warningf("Failed to free program data for process %d\n", process->pid);
        ASSERT(false, "Failed to free program data for process");
        return res;
    }

    kfree(process->stack);
    task_free(process->task);
    process_unlink(process);

    return res;
}

void process_get_arguments(struct process *process, int *argc, char ***argv) {
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_arguments(struct command_argument *root_argument) {
    int i                            = 0;
    struct command_argument *current = root_argument;
    while (current) {
        i++;
        current = current->next;
    }

    return i;
}

int process_inject_arguments(struct process *process, struct command_argument *root_argument) {
    int res = 0;

    struct command_argument *current = root_argument;
    int i                            = 0;

    int argc = process_count_command_arguments(root_argument);

    if (argc == 0) {
        // ASSERT(false, "No arguments to inject");
        res = -EINVARG;
        goto out;
    }

    char **argv = process_malloc(process, sizeof(const char *) * argc);
    if (!argv) {
        ASSERT(false, "Failed to allocate memory for arguments");
        res = -ENOMEM;
        goto out;
    }

    while (current) {
        char *argument_str = process_malloc(process, sizeof(current->argument));
        if (!argument_str) {
            ASSERT(false, "Failed to allocate memory for argument string");
            res = -ENOMEM;
            goto out;
        }

        strncpy(argument_str, current->argument, sizeof(current->argument));
        argv[i] = argument_str;
        current = current->next;
        i++;
    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;

out:
    return res;
}

void process_free(struct process *process, void *ptr) {
    struct process_allocation *allocation = process_get_allocation_by_address(process, ptr);
    if (!allocation) {
        ASSERT(false, "Failed to find allocation for address");
        return;
    }

    int res = paging_map_to(process->task->page_directory, allocation->ptr, allocation->ptr,
                            paging_align_address((char *)allocation->ptr + allocation->size), 0x00);
    if (res < 0) {
        ASSERT(false, "Failed to unmap memory");
        return;
    }

    for (size_t i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr) {
            process->allocations[i].ptr  = NULL;
            process->allocations[i].size = 0;

            break;
        }
    }

    kfree(ptr);
}

void *process_malloc(struct process *process, size_t size) {
    void *ptr = kzalloc(size);
    if (!ptr) {
         ASSERT(false, "Failed to allocate memory for process");
        goto out_error;
    }

    int index = process_find_free_allocation_slot(process);
    if (index < 0) {
         ASSERT(false, "Failed to find free allocation slot");
        goto out_error;
    }

    int res = paging_map_to(process->task->page_directory, ptr, ptr, paging_align_address((char *)ptr + size),
                            PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                                PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    if (res < 0) {
         ASSERT(false, "Failed to map memory for process");
        goto out_error;
    }

    process->allocations[index].ptr  = ptr;
    process->allocations[index].size = size;

    return ptr;

out_error:
    if (ptr) {
        kfree(ptr);
    }
    return NULL;
}

static int process_load_binary(const char *file_name, struct process *process) {
    dbgprintf("Loading binary %s\n", file_name);

    int res       = 0;
    void *program = NULL;
    int fd        = fopen(file_name, "r");
    if (!fd) {
        warningf("Failed to open file %s\n", file_name);
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != ALL_OK) {
        ASSERT(false, "Failed to get file stat");
        res = -EIO;
        goto out;
    }

    program = kzalloc(stat.size);
    if (!program) {
        ASSERT(false, "Failed to allocate memory for program");
        res = -ENOMEM;
        goto out;
    }

    if (fread(program, stat.size, 1, fd) != 1) {
        ASSERT(false, "Failed to read file");
        res = -EIO;
        goto out;
    }

    process->file_type = PROCESS_FILE_TYPE_BINARY;
    process->pointer   = program;
    process->size      = stat.size;

    dbgprintf("Loaded binary %s to %x\n", file_name, program);
    dbgprintf("Program size: %d\n", stat.size);

out:
    if (res < 0) {
        if (program) {
            kfree(program);
        }
    }
    fclose(fd);
    return res;
}

static int process_load_elf(const char *file_name, struct process *process) {
    int res                   = 0;
    struct elf_file *elf_file = NULL;

    res = elf_load(file_name, &elf_file);
    if (ISERR(res)) {
        warningf("Failed to load ELF file\n");
        warningf("Error code: %d\n", res);
        goto out;
    }

    process->file_type = PROCESS_FILE_TYPE_ELF;
    process->elf_file  = elf_file;

out:
    return res;
}

static int process_load_data(const char *file_name, struct process *process) {
    dbgprintf("Loading data for process %s\n", file_name);
    int res = 0;

    res = process_load_elf(file_name, process);
    if (res == -EINFORMAT) {
        warningf("Failed to load ELF file, trying to load as binary\n");
        res = process_load_binary(file_name, process);
    }

    return res;
}

static int process_map_binary(struct process *process) {
    int res = 0;
    paging_map_to(process->task->page_directory, (void *)PROGRAM_VIRTUAL_ADDRESS, process->pointer,
                  paging_align_address((char *)process->pointer + process->size),
                  PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                      PAGING_DIRECTORY_ENTRY_SUPERVISOR);

    return res;
}

static int process_map_elf(struct process *process) {
    int res                   = 0;
    struct elf_file *elf_file = process->elf_file;
    struct elf_header *header = elf_header(elf_file);
    struct elf32_phdr *phdrs  = elf_pheader(header);

    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr *phdr = &phdrs[i];
        void *phdr_phys_address = elf_phdr_phys_address(elf_file, phdr);
        int flags               = PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR;

        if (phdr->p_flags & PF_W) {
            flags |= PAGING_DIRECTORY_ENTRY_IS_WRITABLE;
        }

        res = paging_map_to(process->task->page_directory, paging_align_to_lower_page((void *)phdr->p_vaddr),
                            paging_align_to_lower_page(phdr_phys_address),
                            paging_align_address((char *)phdr_phys_address + phdr->p_memsz), flags);
        if (ISERR(res)) {
            ASSERT(false, "Failed to map ELF file");
            break;
        }
    }

    return res;
}

int process_map_memory(struct process *process) {
    int res = 0;
    switch (process->file_type) {
    case PROCESS_FILE_TYPE_ELF:
        res = process_map_elf(process);
        break;
    case PROCESS_FILE_TYPE_BINARY:
        res = process_map_binary(process);
        break;
    default:
        panic("Unknown process file type");
        break;
    }

    ASSERT(res >= 0, "Failed to map memory for process");

    res = paging_map_to(process->task->page_directory,
                        (char *)PROGRAM_VIRTUAL_STACK_ADDRESS_END, // stack grows down
                        process->stack, paging_align_address((char *)process->stack + USER_PROGRAM_STACK_SIZE),
                        PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                            PAGING_DIRECTORY_ENTRY_SUPERVISOR);

out:
    return res;
}

int process_get_free_slot() {
    for (size_t i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == 0) {
            return i;
        }
    }

    return -EINSTKN;
}

int process_load_switch(const char *file_name, struct process **process) {
    dbgprintf("Loading and switching process %s\n", file_name);

    int res = process_load(file_name, process);
    if (res == 0) {
        process_switch(*process);
    }

    return res;
}

int process_load(const char *file_name, struct process **process) {
    dbgprintf("Loading process %s\n", file_name);
    int res          = 0;
    int process_slot = process_get_free_slot();
    if (process_slot < 0) {
        ASSERT(false, "Failed to get free process slot");
        warningf("Failed to get free process slot\n");
        res = -EINSTKN;
        goto out;
    }

    res = process_load_for_slot(file_name, process, process_slot);

out:
    return res;
}

int process_load_for_slot(const char *file_name, struct process **process, uint16_t slot) {
    int res           = 0;
    struct task *task = 0;
    struct process *proc;
    void *program_stack_pointer = 0;

    dbgprintf("Loading process %s to slot %d\n", file_name, slot);

    if (process_get(slot) != 0) {
        warningf("Process slot is not empty\n");
        ASSERT(false, "Process slot is not empty");
        res = -EINSTKN;
        goto out;
    }
    proc = kzalloc(sizeof(struct process));
    if (!proc) {
        warningf("Failed to allocate memory for process\n");
        ASSERT(false, "Failed to allocate memory for process");
        res = -ENOMEM;
        goto out;
    }

    process_init(proc);
    res = process_load_data(file_name, proc);
    if (res < 0) {
        warningf("Failed to load data for process\n");
        ASSERT(false, "Failed to load data for process");
        goto out;
    }

    program_stack_pointer = kzalloc(USER_PROGRAM_STACK_SIZE);
    if (!program_stack_pointer) {
        warningf("Failed to allocate memory for program stack\n");
        ASSERT(false, "Failed to allocate memory for program stack");
        res = -ENOMEM;
        goto out;
    }

    strncpy(proc->file_name, file_name, sizeof(proc->file_name));
    proc->stack = program_stack_pointer;
    proc->pid   = slot;

    dbgprintf("Process %s stack pointer is %x and process id is %d\n", file_name, program_stack_pointer, slot);

    task = task_create(proc);
    if (ERROR_I(task) == 0) {
        warningf("Failed to create task\n");
        ASSERT(false, "Failed to create task");
        res = -ENOMEM;
        goto out;
    }

    proc->task = task;

    res = process_map_memory(proc);
    if (res < 0) {
        warningf("Failed to map memory for process\n");
        ASSERT(false, "Failed to map memory for process");
        goto out;
    }

    *process = proc;

    processes[slot] = proc;

out:
    if (ISERR(res)) {
        if (proc && proc->task) {
            task_free(proc->task);
        }

        // TODO: free process data
    }
    return res;
}