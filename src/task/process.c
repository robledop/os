#include "process.h"

#include <idt.h>
#include "assert.h"
#include "elfloader.h"
#include "file.h"
#include "kernel.h"
#include "kernel_heap.h"
#include "memory.h"
#include "paging.h"
#include "scheduler.h"
#include "serial.h"
#include "status.h"
#include "string.h"
#include "task.h"
#include "vga_buffer.h"

int process_get_child_count(const struct process *process)
{
    int count                   = 0;
    const struct process *child = process->children;
    while (child) {
        count++;
        child = child->next;
    }
    return count;
}

struct process *find_child_process_by_state(const struct process *parent, const enum PROCESS_STATE state)
{
    // Traverse the linked list of children
    struct process *child = parent->children;
    while (child) {
        if (child->state == state) {
            return child;
        }
        child = child->next;
    }
    return nullptr;
}

struct process *find_child_process_by_pid(const struct process *parent, const int pid)
{
    // Traverse the linked list of children
    struct process *child = parent->children;
    while (child) {
        if (child->pid == pid) {
            return child;
        }
        child = child->next;
    }
    return nullptr;
}

int process_add_child(struct process *parent, struct process *child)
{
    if (!parent || !child) {
        return -EINVARG;
    }

    if (parent->children == nullptr) {
        parent->children = child;
        return ALL_OK;
    }

    struct process *current = parent->children;
    while (current->next) {
        current = current->next;
    }

    current->next = child;
    return ALL_OK;
}

// TODO: free process data
int process_remove_child(struct process *parent, struct process *child)
{
    if (!parent || !child) {
        return -EINVARG;
    }

    if (parent->children == child) {
        parent->children = child->next;
        return ALL_OK;
    }

    struct process *current = parent->children;
    while (current->next) {
        if (current->next == child) {
            current->next = child->next;
            return ALL_OK;
        }
        current = current->next;
    }

    return -ENOENT;
}

static int process_find_free_allocation_slot(const struct process *process)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
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

static struct process_allocation *process_get_allocation_by_address(struct process *process, const void *address)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == address) {
            return &process->allocations[i];
        }
    }

    return nullptr;
}

int process_free_allocations(struct process *process)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        process_free(process, process->allocations[i].ptr);
    }

    return 0;
}

int process_free_program_data(const struct process *process)
{
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

int process_terminate(struct process *process)
{
    process->state = ZOMBIE;

    int res = process_free_allocations(process);
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

    scheduler_unlink_process(process);

    return res;
}

int process_count_command_arguments(const struct command_argument *root_argument)
{
    int i                                  = 0;
    const struct command_argument *current = root_argument;
    while (current) {
        i++;
        current = current->next;
    }

    return i;
}

int process_inject_arguments(struct process *process, const struct command_argument *root_argument)
{
    int res = 0;
    ASSERT(root_argument->current_directory, "Current directory is not set");

    const struct command_argument *current = root_argument;
    int i                                  = 0;

    const int argc = process_count_command_arguments(root_argument);

    if (argc == 0) {
        ASSERT(false, "No arguments to inject");
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
    process_set_current_directory(process, root_argument->current_directory);

out:
    return res;
}

void process_free(struct process *process, void *ptr)
{
    struct process_allocation *allocation = process_get_allocation_by_address(process, ptr);
    if (!allocation) {
        ASSERT(false, "Failed to find allocation for address");
        return;
    }

    const int res = paging_map_to(process->task->page_directory,
                                  allocation->ptr,
                                  allocation->ptr,
                                  paging_align_address((char *)allocation->ptr + allocation->size),
                                  PAGING_DIRECTORY_ENTRY_UNMAPPED);

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

void *process_calloc(struct process *process, const size_t nmemb, const size_t size)
{
    void *ptr = process_malloc(process, nmemb * size);
    if (!ptr) {
        return NULL;
    }

    memset(ptr, 0x00, nmemb * size);
    return ptr;
}

// Allocate memory accessible by the process
void *process_malloc(struct process *process, const size_t size)
{
    void *ptr = kmalloc(size);
    if (!ptr) {
        ASSERT(false, "Failed to allocate memory for process");
        goto out_error;
    }

    const int index = process_find_free_allocation_slot(process);
    if (index < 0) {
        ASSERT(false, "Failed to find free allocation slot");
        goto out_error;
    }

    const int res = paging_map_to(process->task->page_directory,
                                  ptr,
                                  ptr,
                                  paging_align_address((char *)ptr + size),
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

// ReSharper disable once CppDFAUnreachableFunctionCall
static int process_load_binary(const char *file_name, struct process *process)
{
    dbgprintf("Loading binary %s\n", file_name);

    int res      = 0;
    const int fd = fopen(file_name, "r");
    if (!fd) {
        warningf("Failed to open file %s\n", file_name);
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != ALL_OK) {
        warningf("Failed to get file stat\n");
        res = -EIO;
        goto out;
    }

    auto const program = kzalloc(stat.size);
    if (!program) {
        ASSERT(false, "Failed to allocate memory for program");
        res = -ENOMEM;
        goto out;
    }

    if (fread(program, stat.size, 1, fd) != 1) {
        warningf("Failed to read file\n");
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

static int process_load_elf(const char *file_name, struct process *process)
{
    int res                   = 0;
    struct elf_file *elf_file = nullptr;

    res = elf_load(file_name, &elf_file);
    if (ISERR(res)) {
        warningf("Failed to load ELF file\n");
        warningf("Error code: %d\n", res);
        goto out;
    }

    process->file_type = PROCESS_FILE_TYPE_ELF;
    process->elf_file  = elf_file;
    process->size      = elf_file->in_memory_size;

out:
    return res;
}

int process_load_data(const char *file_name, struct process *process)
{
    dbgprintf("Loading data for process %s\n", file_name);
    int res = 0;

    res = process_load_elf(file_name, process);
    // ReSharper disable once CppDFAConstantConditions
    if (res == -EINFORMAT) {
        warningf("Failed to load ELF file, trying to load as binary\n");
        res = process_load_binary(file_name, process);
    }

    return res;
}

static int process_map_binary(const struct process *process)
{
    return paging_map_to(process->task->page_directory,
                         (void *)PROGRAM_VIRTUAL_ADDRESS,
                         process->pointer,
                         paging_align_address((char *)process->pointer + process->size),
                         PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                             PAGING_DIRECTORY_ENTRY_SUPERVISOR);
}

static int process_unmap_binary(const struct process *process)
{
    return paging_map_to(process->task->page_directory,
                         (void *)PROGRAM_VIRTUAL_ADDRESS,
                         process->pointer,
                         paging_align_address((char *)process->pointer + process->size),
                         PAGING_DIRECTORY_ENTRY_UNMAPPED);
}

static int process_map_elf(const struct process *process)
{
    int res                         = 0;
    const struct elf_file *elf_file = process->elf_file;
    struct elf_header *header       = elf_header(elf_file);
    const struct elf32_phdr *phdrs  = elf_pheader(header);

    for (int i = 0; i < header->e_phnum; i++) {
        const struct elf32_phdr *phdr = &phdrs[i];
        void *phdr_phys_address       = elf_phdr_phys_address(elf_file, phdr);
        int flags                     = PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR;

        if (phdr->p_flags & PF_W) {
            flags |= PAGING_DIRECTORY_ENTRY_IS_WRITABLE;
        }

        res = paging_map_to(process->task->page_directory,
                            paging_align_to_lower_page((void *)phdr->p_vaddr),
                            paging_align_to_lower_page(phdr_phys_address),
                            paging_align_address((char *)phdr_phys_address + phdr->p_memsz),
                            flags);
        if (ISERR(res)) {
            ASSERT(false, "Failed to map ELF file");
            break;
        }
    }

    return res;
}

static int process_unmap_elf(const struct process *process)
{
    int res                         = 0;
    const struct elf_file *elf_file = process->elf_file;
    struct elf_header *header       = elf_header(elf_file);
    const struct elf32_phdr *phdrs  = elf_pheader(header);

    for (int i = 0; i < header->e_phnum; i++) {
        const struct elf32_phdr *phdr = &phdrs[i];
        void *phdr_phys_address       = elf_phdr_phys_address(elf_file, phdr);

        res = paging_map_to(process->task->page_directory,
                            paging_align_to_lower_page((void *)phdr->p_vaddr),
                            paging_align_to_lower_page(phdr_phys_address),
                            paging_align_address((char *)phdr_phys_address + phdr->p_memsz),
                            PAGING_DIRECTORY_ENTRY_UNMAPPED);
        if (ISERR(res)) {
            ASSERT(false, "Failed to unmap ELF file");
            break;
        }
    }

    return res;
}

int process_map_memory(const struct process *process)
{
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

    // Map stack
    res = paging_map_to(process->task->page_directory,
                        (char *)USER_STACK_BOTTOM, // stack grows down
                        process->stack,
                        paging_align_address((char *)process->stack + USER_STACK_SIZE),
                        PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                            PAGING_DIRECTORY_ENTRY_SUPERVISOR);


out:
    return res;
}

int process_unmap_memory(const struct process *process)
{
    int res = 0;
    switch (process->file_type) {
    case PROCESS_FILE_TYPE_ELF:
        res = process_unmap_elf(process);
        break;
    case PROCESS_FILE_TYPE_BINARY:
        res = process_unmap_binary(process);
        break;
    default:
        panic("Unknown process file type");
        break;
    }

    ASSERT(res >= 0, "Failed to unmap memory for process");

    res = paging_map_to(process->task->page_directory,
                        (char *)USER_STACK_BOTTOM, // stack grows down
                        process->stack,
                        paging_align_address((char *)process->stack + USER_STACK_SIZE),
                        PAGING_DIRECTORY_ENTRY_UNMAPPED);
    return res;
}


int process_load_switch(const char *file_name, struct process **process)
{
    dbgprintf("Loading and switching process %s\n", file_name);

    const int res = process_load(file_name, process);
    if (res == 0) {
        scheduler_queue_task((*process)->task);
        scheduler_set_current_process(*process);
    }

    return res;
}

int process_load(const char *file_name, struct process **process)
{
    dbgprintf("Loading process %s\n", file_name);
    int res       = 0;
    const int pid = scheduler_get_free_pid();
    if (pid < 0) {
        warningf("Failed to get free process slot\n");
        ASSERT(false, "Failed to get free process slot");
        res = -EINSTKN;
        goto out;
    }

    res = process_load_for_slot(file_name, process, pid);

out:
    return res;
}

// struct process *process_create(const char *file_name)
// {
//     int res                     = 0;
//     struct task *task           = nullptr;
//     struct process *proc        = nullptr;
//     void *program_stack_pointer = nullptr;
//
//     proc = kzalloc(sizeof(struct process));
//     if (!proc) {
//         warningf("Failed to allocate memory for process\n");
//         ASSERT(false, "Failed to allocate memory for process");
//         res = -ENOMEM;
//         goto out;
//     }
//
//     res = process_load_data(file_name, proc);
//     if (res < 0) {
//         warningf("Failed to load data for process\n");
//         goto out;
//     }
//
//     program_stack_pointer = kzalloc(USER_STACK_SIZE);
//     if (!program_stack_pointer) {
//         warningf("Failed to allocate memory for program stack\n");
//         ASSERT(false, "Failed to allocate memory for program stack");
//         res = -ENOMEM;
//         goto out;
//     }
//
//     strncpy(proc->file_name, file_name, sizeof(proc->file_name));
//     proc->stack = program_stack_pointer;
//     // proc->pid   = pid;
//
//
//     task = task_create(proc);
//     if (ERROR_I(task) == 0) {
//         warningf("Failed to create task\n");
//         ASSERT(false, "Failed to create task");
//         res = -ENOMEM;
//         goto out;
//     }
//
//     proc->task = task;
//
//     res = process_map_memory(proc);
//     if (res < 0) {
//         warningf("Failed to map memory for process\n");
//         ASSERT(false, "Failed to map memory for process");
//         goto out;
//     }
//
// out:
//     if (ISERR(res)) {
//         if (proc && proc->task) {
//             task_free(proc->task);
//         }
//
//         // TODO: free process data
//     }
//     return proc;
// }


int process_load_for_slot(const char *file_name, struct process **process, const uint16_t pid)
{
    int res                     = 0;
    struct task *task           = nullptr;
    struct process *proc        = nullptr;
    void *program_stack_pointer = nullptr;

    dbgprintf("Loading process %s to slot %d\n", file_name, pid);

    if (scheduler_get_process(pid) != nullptr) {
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


    res = process_load_data(file_name, proc);
    if (res < 0) {
        warningf("Failed to load data for process\n");
        goto out;
    }

    program_stack_pointer = kzalloc(USER_STACK_SIZE);
    if (!program_stack_pointer) {
        warningf("Failed to allocate memory for program stack\n");
        ASSERT(false, "Failed to allocate memory for program stack");
        res = -ENOMEM;
        goto out;
    }

    strncpy(proc->file_name, file_name, sizeof(proc->file_name));
    proc->stack = program_stack_pointer;
    proc->pid   = pid;

    dbgprintf("Process %s stack pointer is %x and process id is %d\n", file_name, program_stack_pointer, pid);

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

    scheduler_set_process(pid, proc);

out:
    if (ISERR(res)) {
        if (proc && proc->task) {
            task_free(proc->task);
        }

        // TODO: free process data
    }
    return res;
}

int process_set_current_directory(struct process *process, const char *directory)
{
    if (!process || !directory || strlen(directory) == 0) {
        return -EINVARG;
    }

    if (process->current_directory == NULL) {
        process->current_directory = process_malloc(process, MAX_PATH_LENGTH);
    }

    strncpy(process->current_directory, directory, MAX_PATH_LENGTH);

    return ALL_OK;
}

int process_wait_pid(struct process *process, const int pid)
{
    ENTER_CRITICAL();

    if (process_get_child_count(process) == 0) {
        LEAVE_CRITICAL();
        return -1;
    }

    struct process *child = nullptr;

    if (pid == -1) {
        child = find_child_process_by_state(process, ZOMBIE);
        if (child == nullptr) {
            process->state    = WAITING;
            process->wait_pid = pid;
            LEAVE_CRITICAL();
            schedule();
            return -1;
        }
    }

    if (pid > 0) {
        child = find_child_process_by_pid(process, pid);
    }

    if (child == nullptr) {
        process->state    = RUNNING;
        process->wait_pid = 0;
        LEAVE_CRITICAL();
        return -1;
    }

    if (child->state == ZOMBIE || child->state == TERMINATED) {
        process_remove_child(process, child);
        scheduler_unlink_process(child);
        task_free(child->task);

        const int status  = child->exit_code;
        process->state    = RUNNING;
        process->wait_pid = 0;
        LEAVE_CRITICAL();
        return status;
    }

    // No child has terminated; block the parent process
    process->state    = WAITING;
    process->wait_pid = pid;
    LEAVE_CRITICAL();

    schedule(); // Context switch to another process
    return -1;  // No child to wait for
}

int process_copy_allocations(struct process *dest, const struct process *src)
{
    memset(dest->allocations, 0, sizeof(dest->allocations));

    for (size_t i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (src->allocations[i].ptr) {
            void *ptr = process_malloc(dest, src->allocations[i].size);
            if (!ptr) {
                return -ENOMEM;
            }

            memcpy(ptr, src->allocations[i].ptr, src->allocations[i].size);
            dest->allocations[i].ptr = ptr;
        }
    }

    return ALL_OK;
}

void process_copy_stack(struct process *dest, const struct process *src)
{
    dest->stack = kzalloc(USER_STACK_SIZE);
    memcpy(dest->stack, src->stack, USER_STACK_SIZE);
}

void process_copy_file_info(struct process *dest, const struct process *src)
{
    memcpy(dest->file_name, src->file_name, sizeof(src->file_name));
    dest->file_type = src->file_type;
    if (dest->file_type == PROCESS_FILE_TYPE_ELF) {
        dest->elf_file             = kzalloc(sizeof(struct elf_file));
        dest->elf_file->elf_memory = kzalloc(src->elf_file->in_memory_size);
        memcpy(dest->elf_file->elf_memory, src->elf_file->elf_memory, src->elf_file->in_memory_size);
        dest->elf_file->in_memory_size = src->elf_file->in_memory_size;
        strncpy(dest->elf_file->filename, src->elf_file->filename, sizeof(src->elf_file->filename));
        dest->elf_file->virtual_base_address = src->elf_file->virtual_base_address;
        dest->elf_file->virtual_end_address  = src->elf_file->virtual_end_address;

        const int res = elf_process_loaded(dest->elf_file);
        if (res < 0) {
            panic("Failed to process loaded ELF file");
        }

    } else {
        dest->pointer = kzalloc(src->size);
        memcpy(dest->pointer, src->pointer, src->size);
    }
    dest->size = src->size;
}

void process_copy_arguments(struct process *dest, const struct process *src)
{
    dest->current_directory = process_malloc(dest, MAX_PATH_LENGTH);
    memcpy(dest->current_directory, src->current_directory, MAX_PATH_LENGTH);

    dest->arguments.argv = process_malloc(dest, sizeof(char *) * src->arguments.argc);
    for (int i = 0; i < src->arguments.argc; i++) {
        dest->arguments.argv[i] = process_malloc(dest, strlen(src->arguments.argv[i]) + 1);
        strncpy(dest->arguments.argv[i], src->arguments.argv[i], strlen(src->arguments.argv[i]) + 1);
    }
}

void process_copy_task(struct process *dest, const struct process *src)
{
    struct task *task = task_create(dest);
    task_copy_registers(task, src->task);
    dest->task          = task;
    dest->task->process = dest;
    dest->task->tty     = src->task->tty;
}

struct process *process_clone(struct process *process)
{
    struct process *clone = kzalloc(sizeof(struct process));
    if (!clone) {
        panic("Failed to allocate memory for process clone");
        return nullptr;
    }

    const int pid = scheduler_get_free_pid();
    if (pid < 0) {
        kfree(clone);
        panic("No free process slot");
        return nullptr;
    }

    clone->pid    = pid;
    clone->parent = process;

    // This is not super efficient

    process_copy_file_info(clone, process);
    process_copy_task(clone, process);
    process_copy_stack(clone, process);
    process_copy_arguments(clone, process);
    process_map_memory(clone);
    process_copy_allocations(clone, process);

    process_add_child(process, clone);
    scheduler_set_process(clone->pid, clone);
    scheduler_queue_task(clone->task);

    return clone;
}

// struct process *process_replace(const struct process *parent, const char *file_name)
// {
//     struct process *child = process_create(file_name);
//     child->parent         = parent->parent;
//     child->pid            = parent->pid;
//
//     // TODO: Copy file descriptors
//
//     return child;
// }