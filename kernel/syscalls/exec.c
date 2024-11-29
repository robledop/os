#include <kernel.h>
#include <kernel_heap.h>
#include <printf.h>
#include <scheduler.h>
#include <spinlock.h>
#include <string.h>
#include <syscall.h>
#include <thread.h>
#include <vfs.h>

spinlock_t exec_lock = 0;


// TODO: Simplify this
// TODO: Fix memory leaks
// int exec(const char *path, const char *argv[])
void *sys_exec(struct interrupt_frame *frame)
{
    spin_lock(&exec_lock);

    const void *path_ptr  = thread_peek_stack_item(scheduler_get_current_thread(), 1);
    void *virtual_address = thread_peek_stack_item(scheduler_get_current_thread(), 0);

    char **args_ptr = nullptr;

    if (virtual_address) {
        args_ptr = thread_virtual_to_physical_address(scheduler_get_current_thread(), virtual_address);
    }

    char path[MAX_PATH_LENGTH] = {0};
    copy_string_from_thread(scheduler_get_current_thread(), path_ptr, path, sizeof(path));

    char *args[256] = {nullptr};
    if (args_ptr) {
        int argc = 0;
        for (int i = 0; i < 256; i++) {
            if (args_ptr[i] == nullptr) {
                break;
            }
            char *arg = kzalloc(256);
            copy_string_from_thread(scheduler_get_current_thread(), args_ptr[i], arg, 256);
            args[i] = arg;
            argc++;
        }
    }

    struct command_argument *root_argument = kzalloc(sizeof(struct command_argument));
    strncpy(root_argument->argument, path, sizeof(root_argument->argument));
    strncpy(root_argument->current_directory,
            scheduler_get_current_thread()->process->current_directory,
            sizeof(root_argument->current_directory));

    struct command_argument *arguments = parse_command(args);
    root_argument->next                = arguments;

    // Free the arguments
    for (int i = 0; i < 256; i++) {
        if (args[i] == nullptr) {
            break;
        }
        kfree(args[i]);
    }

    struct process *process = scheduler_get_current_process();
    process_free_allocations(process);
    process_free_program_data(process);
    kfree(process->stack);
    process->stack = nullptr;
    paging_free_directory(process->page_directory);
    thread_free(process->thread);
    process->thread = nullptr;
    process_unmap_memory(process);

    char full_path[MAX_PATH_LENGTH] = {0};
    if (istrncmp(path, "/", 1) != 0) {
        strcat(full_path, "/bin/");
        strcat(full_path, path);
    } else {
        strcat(full_path, path);
    }

    const int res = process_load_data(full_path, process);
    if (res < 0) {
        printf("Result: %d\n", res);
        spin_unlock(&exec_lock);
        return (void *)res;
    }
    void *program_stack_pointer = kzalloc(USER_STACK_SIZE);
    strncpy(process->file_name, full_path, sizeof(process->file_name));
    process->stack        = program_stack_pointer; // Physical address of the stack for the process
    struct thread *thread = thread_create(process);
    if (ISERR(thread)) {
        panic("Failed to create thread");
    }
    process->thread = thread;
    process_map_memory(process);

    process_inject_arguments(process, root_argument);

    scheduler_set_process(process->pid, process);
    scheduler_queue_thread(thread);

    spin_unlock(&exec_lock);

    return nullptr;
}
