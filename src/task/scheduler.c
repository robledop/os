#include <assert.h>
#include <config.h>
#include <idt.h>
#include <kernel_heap.h>
#include <memory.h>
#include <pic.h>
#include <pit.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <status.h>

#include "../../user/stdlib/include/string.h"

uint32_t milliseconds                           = 0;
struct process *current_process                 = nullptr;
static struct process *processes[MAX_PROCESSES] = {nullptr};

struct task *current_task = nullptr;
struct task *task_tail    = nullptr;
struct task *task_head    = nullptr;
struct task *idle_task;

struct process *scheduler_get_current_process()
{
    return current_process;
}

struct process *scheduler_get_process(const int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES) {
        warningf("Invalid process id: %d\n", pid);
        ASSERT(false, "Invalid process id");
        return nullptr;
    }

    return processes[pid];
}

struct process *scheduler_set_process(const int pid, struct process *process)
{
    if (pid < 0 || pid >= MAX_PROCESSES) {
        warningf("Invalid process id: %d\n", pid);
        ASSERT(false, "Invalid process id");
        return nullptr;
    }

    processes[pid] = process;
    return process;
}

void scheduler_set_current_process(struct process *process)
{
    ASSERT(process, "Invalid process");
    current_process = process;
}

/// @brief Switch to any process that is in the RUNNING state.
/// If no process is found, start the shell.
void scheduler_switch_to_any()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && processes[i]->priority != 0) {
            scheduler_set_current_process(processes[i]);
            return;
        }
    }

    start_shell(0);
}

// TODO: Unlink the child process when the parent does not wait for it
void scheduler_unlink_process(const struct process *process)
{
    processes[process->pid] = nullptr;

    if (current_process == process) {
        scheduler_switch_to_any();
    }
}

int scheduler_get_free_pid()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] == nullptr) {
            return i;
        }
    }

    return -EINSTKN;
}

struct task *scheduler_get_current_task()
{
    return current_task;
}

int scheduler_get_task_count()
{
    const struct task *task = task_head;
    int count               = 0;
    while (task) {
        count++;
        task = task->next;
    }
    return count;
}

struct task *scheduler_get_runnable_task()
{
    struct task *task = task_head;
    while (task) {
        if (task->process->state == RUNNING) {
            return task;
        }
        task = task->next;
    }
    return nullptr;
}

struct task *scheduler_get_next_task()
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

void scheduler_queue_task(struct task *task)
{
    if (!task_head) {
        task_head    = task;
        task_tail    = task;
        current_task = task;
    } else {
        task_tail->next = task;
        task->prev      = task_tail;
        task_tail       = task;
    }
}

void scheduler_unqueue_task(const struct task *task)
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
        current_task = scheduler_get_next_task();
    }
}


void scheduler_save_current_task(const struct interrupt_frame *interrupt_frame)
{
    struct task *task = scheduler_get_current_task();
    if (!task) {
        panic("No current task");
    }

    task_save_state(task, interrupt_frame);
}

// void scheduler_run_first_task()
// {
//     if (!current_task) {
//         panic("No tasks to run");
//     }
//
//     dbgprintf("Running first ever task %x\n", current_task);
//
//     scheduler_switch_task(task_head);
//     scheduler_run_task_in_user_mode(&task_head->registers);
// }

/// @brief Set the task as the current task and switch to its page directory
/// @param task the task to run
/// @return ALL_OK on success, error code otherwise
int scheduler_switch_task(struct task *task)
{
    current_task = task;
    paging_switch_directory(task->page_directory);
    return ALL_OK;
}

/// @brief Set the user mode segments and switch to the current task's page directory
int scheduler_switch_current_task_page()
{
    set_user_mode_segments();
    paging_switch_directory(current_task->page_directory);

    return ALL_OK;
}

void scheduler_idle_task()
{
    // Enable interrupts, otherwise we will never leave the idle task
    LEAVE_CRITICAL();
    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        kprintf(KYEL "Idle!" KWHT);
        asm("hlt");
    }
}

void scheduler_initialize_idle_task()
{
    idle_task = kzalloc(sizeof(struct task));
    memset(idle_task, 0, sizeof(struct task));
    idle_task->tty                 = 0;
    idle_task->kernel_state.eip    = (uint32_t)&scheduler_idle_task;
    idle_task->kernel_state.cs     = KERNEL_CODE_SELECTOR;
    idle_task->kernel_state.eflags = 0x200; // Enable interrupts
    // idle_task->kernel_state.esp    = (uint32_t)kzalloc(4096) + 4096;

    idle_task->process           = (struct process *)kzalloc(sizeof(struct process));
    idle_task->process->pid      = 0;
    idle_task->process->state    = RUNNING;
    idle_task->process->priority = 0;
    idle_task->process->task     = idle_task;
    strncpy(idle_task->process->file_name, "idle", 5);

    scheduler_set_process(idle_task->process->pid, idle_task->process);
}

void scheduler_run_task_in_kernel_mode(cpu_state_t state)
{
    asm volatile("mov %0, %%eax\n"
                 "jmp *%%eax\n"
                 :
                 : "m"(state.eip)
                 : "%eax");
}

/// @brief Gets the next task and runs it
void schedule()
{
    struct task *next = scheduler_get_next_task();
    if (!next) {
        // No tasks to run, restart the shell
        kprintf("\nRestarting the shell");
        pic_acknowledge();
        start_shell(0);
        return;
    }

    // If the task is terminated or a zombie, remove it from the scheduler
    if (next->process->state == TERMINATED || next->process->state == ZOMBIE) {
        // If the task is a child process, signal the parent that the child has exited
        if (next->process->parent && next->process->parent->state == WAITING &&
            (next->process->parent->wait_pid == next->process->pid || next->process->parent->wait_pid == -1)) {
            next->process->parent->state     = RUNNING;
            next->process->parent->exit_code = next->process->exit_code;
            next->process->parent->wait_pid  = 0;
        }

        scheduler_unqueue_task(next);
        scheduler_unlink_process(next->process);
        pic_acknowledge();
        return;
    }

    auto const process = next->process;

    // TODO: Add a way for the child process to signal to the parent that it has exited
    if (process->state == WAITING) {
        struct process *child;
        if (process->wait_pid == -1) {
            child = find_child_process_by_state(process, process->wait_state);
        } else {
            child = find_child_process_by_pid(process, process->wait_pid);
            if (!child) {
                process->state     = RUNNING;
                process->exit_code = 0;
                process->wait_pid  = 0;
            }
        }
        if (child && child->state == process->wait_state) {
            process->state     = RUNNING;
            process->exit_code = child->exit_code;
            process->wait_pid  = 0;

            if (child->state == TERMINATED || child->state == ZOMBIE) {
                process_remove_child(process, child);
                scheduler_unlink_process(child);
            }
        } else {
            if (child) {
                // If the task is waiting and the child is still running, switch to the child task
                pic_acknowledge();
                scheduler_switch_task(child->task);
                scheduler_run_task_in_user_mode(&child->task->registers);
            } else {
                // Try to find another runnable task, if none, run the idle task
                next = scheduler_get_runnable_task();
                if (next == nullptr) {
                    pic_acknowledge();
                    scheduler_run_task_in_kernel_mode(idle_task->kernel_state);
                    return;
                }
            }
        }
    }

    pic_acknowledge();
    scheduler_switch_task(next);
    scheduler_run_task_in_user_mode(&next->registers);
}

int scheduler_replace(struct process *old, struct process *new)
{
    process_terminate(old);

    processes[old->pid] = new;

    return ALL_OK;
}

void handle_pit_interrupt(int interrupt, uint32_t unused)
{
    milliseconds += 2;
    if (milliseconds >= 10) {
        milliseconds = 0;
        schedule();
    }
    pic_acknowledge();
}

int scheduler_init()
{
    scheduler_initialize_idle_task();
    pit_set_interval(2);
    return idt_register_interrupt_callback(0x20, handle_pit_interrupt);
}

int scheduler_get_processes(struct process_info **proc_info, int *count)
{
    // Count the number of processes
    *count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] != nullptr) {
            (*count)++;
        }
    }

    *proc_info = (struct process_info *)kmalloc(sizeof(struct process_info) * *count);

    for (int i = 0; i < *count; i++) {
        if (processes[i]) {
            struct process_info info = {
                .pid        = processes[i]->pid,
                .priority   = processes[i]->priority,
                .state      = processes[i]->state,
                .wait_state = processes[i]->wait_state,
                .exit_code  = processes[i]->exit_code,
            };
            strncpy(info.file_name, processes[i]->file_name, MAX_PATH_LENGTH);
            memcpy(*proc_info + i, &info, sizeof(struct process_info));
        }
    }

    return 0;
}