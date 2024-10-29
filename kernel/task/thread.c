#include "thread.h"

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

int thread_init(struct thread *thread, struct process *process);

int thread_free(struct thread *thread)
{
    if (thread == NULL) {
        return -EINVARG;
    }

    paging_free_directory(thread->process->page_directory);
    scheduler_unqueue_thread(thread);

    kfree(thread);

    return ALL_OK;
}

struct thread *thread_create(struct process *process)
{
    dbgprintf("Creating thread for process %x\n", process);

    int res = 0;

    auto const thread = (struct thread *)kzalloc(sizeof(struct thread));
    if (!thread) {
        dbgprintf("Failed to allocate memory for thread\n");
        res = -ENOMEM;
        goto out;
    }

    res = thread_init(thread, process);
    if (res != ALL_OK) {
        dbgprintf("Failed to initialize thread\n");
        goto out;
    }


out:
    if (ISERR(res)) {
        thread_free(thread);
        return ERROR(res);
    }

    return thread;
}

/// @brief Set the user mode segments and switch to the thread's page directory
/// @param thread the thread to switch to
int thread_page_thread(const struct thread *thread)
{
    set_user_mode_segments();
    paging_switch_directory(thread->process->page_directory);

    return ALL_OK;
}

int copy_string_from_thread(const struct thread *thread, const void *virtual, void *physical, const int max)
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

    const uint32_t old_entry = paging_get(thread->process->page_directory, tmp);

    paging_map(thread->process->page_directory,
               tmp,
               tmp,
               PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
                   PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(thread->process->page_directory);
    strncpy(tmp, virtual, max);
    kernel_page();
    res = paging_set(thread->process->page_directory, tmp, old_entry);
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

int thread_init(struct thread *thread, struct process *process)
{
    memset(thread, 0, sizeof(struct thread));
    thread->process = process;
    thread->process->page_directory =
        paging_create_directory(PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);

    if (!thread->process->page_directory) {
        dbgprintf("Failed to create page directory for thread %x\n", &thread);
        return -ENOMEM;
    }

    switch (process->file_type) {
    case PROCESS_FILE_TYPE_BINARY:
        thread->registers.ip = PROGRAM_VIRTUAL_ADDRESS;
        break;
    case PROCESS_FILE_TYPE_ELF:
        thread->registers.ip = elf_header(process->elf_file)->e_entry;
        break;
    default:
        panic("Unknown process file type");
        break;
    }

    thread->registers.ss  = USER_DATA_SELECTOR;
    thread->registers.cs  = USER_CODE_SELECTOR;
    thread->registers.esp = USER_STACK_TOP;


    dbgprintf("Thread %x initialized\n", thread);
    dbgprintf("Thread %x page directory %x\n", thread, thread->page_directory->directory_entry);
    dbgprintf("Thread %x registers ip %x\n", thread, thread->registers.ip);

    return ALL_OK;
}

void *thread_peek_stack_item(const struct thread *thread, const int index)
{
    const uint32_t *stack_pointer = (uint32_t *)thread->registers.esp;
    thread_page_thread(thread);
    auto const result = (void *)stack_pointer[index];
    kernel_page();
    return result;
}

void *thread_virtual_to_physical_address(const struct thread *thread, void *virtual_address)
{
    return paging_get_physical_address(thread->process->page_directory, virtual_address);
}

void thread_save_state(struct thread *thread, const struct interrupt_frame *frame)
{
    thread->registers.edi = frame->edi;
    thread->registers.esi = frame->esi;
    thread->registers.ebp = frame->ebp;
    thread->registers.ebx = frame->ebx;
    thread->registers.edx = frame->edx;
    thread->registers.ecx = frame->ecx;
    thread->registers.eax = frame->eax;

    thread->registers.ip    = frame->ip;
    thread->registers.cs    = frame->cs;
    thread->registers.flags = frame->flags;
    thread->registers.esp   = frame->esp;
    thread->registers.ss    = frame->ss;
}

void thread_copy_registers(struct thread *dest, const struct thread *src)
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
