#include <debug.h>
#include <elf.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memory.h>
#include <paging.h>
#include <process.h>
#include <rand.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <string.h>
#include <sys/stat.h>
#include <vfs.h>
#include <x86.h>

struct {
    spinlock_t lock;
    struct process *processes[MAX_PROCESSES];
} process_list;

spinlock_t process_lock = 0;

int process_get_free_pid()
{
    spin_lock(&process_list.lock);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_list.processes[i] == nullptr) {
            spin_unlock(&process_list.lock);
            return i;
        }
    }

    spin_unlock(&process_list.lock);

    return -EINSTKN;
}

struct process *process_get(const int pid)
{
    return process_list.processes[pid];
}

void process_set(const int pid, struct process *process)
{
    process_list.processes[pid] = process;
}

struct process *get_current_process(void)
{
    const struct task *current_task = get_current_task();
    if (current_task) {
        return current_task->process;
    }

    return nullptr;
}

static int process_find_free_allocation_slot(const struct process *process)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == NULL) {
            return i;
        }
    }

    panic("Failed to find free allocation slot");
    return -ENOMEM;
}

static bool process_is_process_pointer(const struct process *process, const void *ptr)
{
    for (size_t i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr) {
            return true;
        }
    }

    return false;
}

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
        if (process->allocations[i].ptr == nullptr) {
            continue;
        }
        process_free(process, process->allocations[i].ptr);
    }

    return 0;
}

int process_free_program_data(const struct process *process)
{
    int res = 0;
    switch (process->file_type) {
    case PROCESS_FILE_TYPE_BINARY:
        if (process->pointer) {
            kfree(process->pointer);
        }
        break;

    case PROCESS_FILE_TYPE_ELF:
        if (process->elf_file) {
            elf_close(process->elf_file);
        }
        break;

    default:
        ASSERT(false, "Unknown process file type");
        res = -EINVARG;
    }
    return res;
}

int process_free_file_descriptors(const struct process *process)
{
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (process->file_descriptors[i]) {
            vfs_close(i);
        }
    }

    return 0;
}

/// @brief Turn the process into a zombie and deallocates its resources
/// The process remains in the process list until the parent process reads the exit code
int process_zombify(struct process *process)
{
    spin_lock(&process_lock);

    process->state = ZOMBIE;

    int res = process_free_allocations(process);
    ASSERT(res == 0, "Failed to free allocations for process");

    res = process_free_file_descriptors(process);
    ASSERT(res == 0, "Failed to free file descriptors for process");

    res = process_free_program_data(process);
    ASSERT(res == 0, "Failed to free program data for process");

    if (process->stack) {
        kfree(process->stack);
    }
    process->stack = nullptr;
    if (process->thread) {
        // thread_free(process->thread);
    }
    process->thread = nullptr;
    if (process->page_directory) {
        paging_free_directory(process->page_directory);
    }
    process->page_directory = nullptr;

    if (strlen(process->file_name) > 0) {
        // scheduler_unlink_process(process);
    }

    spin_unlock(&process_lock);
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
    int res                                = 0;
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

    const int res = paging_map_to(process->page_directory,
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

    const int res = paging_map_to(process->page_directory,
                                  ptr,
                                  ptr,
                                  paging_align_address((char *)ptr + size),
                                  PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                                      PAGING_DIRECTORY_ENTRY_SUPERVISOR); // TODO: Get rid of supervisor flag
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

    void *program = nullptr;

    int res      = 0;
    const int fd = vfs_open(file_name, O_RDONLY);
    if (fd < 0) {
        warningf("Failed to open file %s\n", file_name);
        res = -EIO;
        goto out;
    }

    struct stat fstat;
    res = vfs_stat(fd, &fstat);
    if (res != ALL_OK) {
        warningf("Failed to get file stat\n");
        res = -EIO;
        goto out;
    }

    program = kzalloc(fstat.st_size);
    if (program == nullptr) {
        ASSERT(false, "Failed to allocate memory for program");
        res = -ENOMEM;
        goto out;
    }

    if (vfs_read(program, fstat.st_size, 1, fd) != 1) {
        warningf("Failed to read file\n");
        res = -EIO;
        goto out;
    }

    process->file_type = PROCESS_FILE_TYPE_BINARY;
    process->pointer   = program;
    process->size      = fstat.st_size;

out:
    if (res < 0) {
        if (program != nullptr) {
            kfree(program);
        }
    }
    vfs_close(fd);
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

int process_load_data(const char file_name[static 1], struct process *process)
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
    return paging_map_to(process->page_directory,
                         (void *)PROGRAM_VIRTUAL_ADDRESS,
                         process->pointer,
                         paging_align_address((char *)process->pointer + process->size),
                         PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_IS_WRITABLE |
                             PAGING_DIRECTORY_ENTRY_SUPERVISOR);
}

static int process_unmap_binary(const struct process *process)
{
    return paging_map_to(process->page_directory,
                         (void *)PROGRAM_VIRTUAL_ADDRESS,
                         process->pointer,
                         paging_align_address((char *)process->pointer + process->size),
                         PAGING_DIRECTORY_ENTRY_UNMAPPED);
}

static int process_map_elf(struct process *process)
{
    int res                         = 0;
    const struct elf_file *elf_file = process->elf_file;
    struct elf_header *header       = elf_header(elf_file);
    const struct elf32_phdr *phdrs  = elf_pheader(header);

    for (int i = 0; i < header->e_phnum; i++) {
        const struct elf32_phdr *phdr = &phdrs[i];

        if (phdr->p_type != PT_LOAD) {
            continue; // Skip non-loadable segments
        }

        // Allocate new physical memory for the segment
        void *phys_addr = process_malloc(process, phdr->p_memsz);
        if (phys_addr == NULL) {
            return -ENOMEM;
        }

        int flags =
            PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR; // TODO: Get rid of supervisor

        if (phdr->p_flags & PF_W) {
            flags |= PAGING_DIRECTORY_ENTRY_IS_WRITABLE;
        }

        res = paging_map_to(process->page_directory,
                            paging_align_to_lower_page((void *)phdr->p_vaddr),
                            paging_align_to_lower_page(phys_addr),
                            paging_align_address((char *)phys_addr + phdr->p_memsz),
                            flags);
        if (ISERR(res)) {
            ASSERT(false, "Failed to map ELF segment");
            kfree(phys_addr);
            break;
        }

        // Copy segment data from the ELF file to the allocated memory
        if (phdr->p_filesz > 0) {
            memcpy(phys_addr, (char *)elf_memory(elf_file) + phdr->p_offset, phdr->p_filesz);
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

        res = paging_map_to(process->page_directory,
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

int process_map_memory(struct process *process)
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
    res = paging_map_to(process->page_directory,
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

    if (process->stack == nullptr) {
        return res;
    }

    res = paging_map_to(process->page_directory,
                        (char *)USER_STACK_BOTTOM, // stack grows down
                        process->stack,
                        paging_align_address((char *)process->stack + USER_STACK_SIZE),
                        PAGING_DIRECTORY_ENTRY_UNMAPPED);
    return res;
}


int process_load_enqueue(const char file_name[static 1], struct process **process)
{
    const int res = process_load(file_name, process);
    if (res == 0) {
        (*process)->thread->wakeup_time = -1;
    }

    return res;
}

int process_load(const char file_name[static 1], struct process **process)
{
    int res       = 0;
    const int pid = process_get_free_pid();
    if (pid < 0) {
        warningf("Failed to get free process slot\n");
        ASSERT(false, "Failed to get free process slot");
        res = -EINSTKN;
        goto out;
    }

    res = process_load_for_slot(file_name, process, pid);

    process_set(pid, *process);
out:
    return res;
}

int process_load_for_slot(const char file_name[static 1], struct process **process, const uint16_t pid)
{
    int res                     = 0;
    struct task *thread         = nullptr;
    struct process *proc        = nullptr;
    void *program_stack_pointer = nullptr;

    if (process_get(pid) != nullptr) {
        panic("Process slot is not empty\n");
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

    thread = thread_create(proc);
    if (ISERR(thread)) {
        warningf("Failed to create thread\n");
        ASSERT(false, "Failed to create thread");
        res = -ENOMEM;
        goto out;
    }

    proc->thread  = thread;
    proc->rand_id = rand();

    res = process_map_memory(proc);
    if (res < 0) {
        warningf("Failed to map memory for process\n");
        ASSERT(false, "Failed to map memory for process");
        goto out;
    }

    proc->state = RUNNING;
    memset(proc->file_descriptors, 0, sizeof(proc->file_descriptors));

    *process = proc;

    // scheduler_set_process(pid, proc);

out:
    if (ISERR(res)) {
        if (proc) {
            process_zombify(proc);
            kfree(proc);
        }
    }
    return res;
}

int process_set_current_directory(struct process *process, const char directory[static 1])
{
    if (strlen(directory) == 0) {
        return -EINVARG;
    }

    if (process->current_directory == NULL) {
        process->current_directory = process_malloc(process, MAX_PATH_LENGTH);
    }

    strncpy(process->current_directory, directory, MAX_PATH_LENGTH);

    return ALL_OK;
}

void process_sleep(void *chan, spinlock_t lock)
{
    struct process *p = get_current_process();

    if (lock != process_list.lock) {
        spin_lock(&process_list.lock);
        spin_unlock(&process_list.lock);
    }

    p->thread->wait_channel = chan;
    tasks_block_current(TASK_BLOCKED);

    sched();

    p->thread->wait_channel = nullptr;

    if (lock != process_list.lock) {
        spin_unlock(&process_list.lock);
        spin_lock(&lock);
    }
}

int process_wait_pid(int child_pid)
{
    struct process *p               = nullptr;
    struct process *current_process = get_current_process();
    int children                    = 0;
    spin_lock(&process_list.lock);

    while (true) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            p = process_list.processes[i];
            if (!p || p->parent != current_process) {
                continue;
            };
            children = 1;
            if (p->thread->state == TASK_STOPPED) {
                const int pid = p->pid;
                // kfree(p->stack);
                // paging_free_directory(p->page_directory);
                // p->pid          = -1;
                // p->parent       = nullptr;
                // p->file_name[0] = '\0';
                // p->state        = EMPTY;
                spin_unlock(&process_list.lock);
                return pid;
            }
        }

        if (!children || current_process->killed) {
            spin_unlock(&process_list.lock);
            return -1;
        }

        process_sleep(current_process, process_list.lock);
    }
}

void process_wakeup(const void *wait_channel)
{
    // spin_unlock(&process_list.lock);

    struct process *p = nullptr;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        p = process_list.processes[i];
        if (p && p->thread->state == TASK_BLOCKED && p->thread->wait_channel == wait_channel) {

            wakeup(p->thread);
            return;
        }
    }

    // spin_unlock(&process_list.lock);
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

void process_copy_thread(struct process *dest, const struct process *src)
{
    struct task *thread = thread_create(dest);
    if (ISERR(thread)) {
        panic("Failed to create thread");
    }
    // thread_copy_registers(thread, src->thread);
    dest->thread          = thread;
    dest->thread->process = dest;
}

struct process *process_clone(struct process *process)
{
    struct process *clone = kzalloc(sizeof(struct process));

    if (!clone) {
        panic("Failed to allocate memory for process clone");
        return nullptr;
    }


    // const int pid = scheduler_get_free_pid();
    const int pid = 1;
    if (pid < 0) {
        kfree(clone);
        panic("No free process slot");
        return nullptr;
    }

    clone->pid    = pid;
    clone->parent = process;

    // This is not super efficient

    process_copy_file_info(clone, process);
    process_copy_thread(clone, process);
    process_copy_stack(clone, process);
    process_copy_arguments(clone, process);
    process_map_memory(clone);
    process_copy_allocations(clone, process);

    // scheduler_set_process(clone->pid, clone);
    // scheduler_queue_thread(clone->thread);
    clone->state = RUNNING;

    clone->rand_id = rand();

    return clone;
}

void process_command_argument_free(struct command_argument *argument)
{
    if (argument->next) {
        process_command_argument_free(argument->next);
    }
    kfree(argument);
}

void process_free_file_descriptor(struct process *process, struct file *desc)
{
    process->file_descriptors[desc->index] = nullptr;
    // Do not free device inodes
    if (desc->inode && desc->inode->type != INODE_DEVICE && desc->fs_type != FS_TYPE_RAMFS) {
        if (desc->inode->data) {
            kfree(desc->inode->data);
        }
        kfree(desc->inode);
    }
    kfree(desc);
}

int process_new_file_descriptor(struct process *process, struct file **desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (process->file_descriptors[i] == nullptr) {
            struct file *desc = kzalloc(sizeof(struct file));
            if (desc == nullptr) {
                panic("Failed to allocate memory for file descriptor\n");
                res = -ENOMEM;
                break;
            }

            desc->index                  = i;
            process->file_descriptors[i] = desc;
            *desc_out                    = desc;
            res                          = 0;
            break;
        }
    }

    return res;
}

struct file *process_get_file_descriptor(const struct process *process, const uint32_t index)
{
    if (index > MAX_FILE_DESCRIPTORS - 1) {
        return nullptr;
    }

    return process->file_descriptors[index];
}
