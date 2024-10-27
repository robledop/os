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

// How often the PIT should interrupt
#define PIT_INTERVAL 1 // 1ms
// How often the scheduler should run
#define TIME_SLICE 10 // 10ms

// Milliseconds since boot
uint32_t jiffies                                = 0;
struct process *current_process                 = nullptr;
static struct process *processes[MAX_PROCESSES] = {nullptr};

struct task *current_task = nullptr;
struct task *task_tail    = nullptr;
struct task *task_head    = nullptr;
struct task *idle_task;
void scheduler_idle_task();

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
            scheduler_switch_task(current_process->task);
            scheduler_run_task_in_user_mode(&current_process->task->registers);
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

    if (task->next) {
        task->next->prev = task->prev;
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

    if (interrupt_frame->cs == KERNEL_CODE_SELECTOR) {
        task_save_state(idle_task, interrupt_frame);
    } else {
        task_save_state(task, interrupt_frame);
    }
}

/// @brief Set the task as the current task and switch to its page directory
/// @param task the task to run
/// @return ALL_OK on success, error code otherwise
int scheduler_switch_task(struct task *task)
{
    current_task = task;
    set_user_mode_segments();
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


void scheduler_run_task_in_kernel_mode(uint32_t eip)
{
    kernel_page();

    // TODO: HACK. Need a proper way of restoring the stack. 180 is the value needed right now, but that will certainly
    // change. This must not be hardcoded
    asm volatile("mov %0, %%eax\n"
                 "sti\n"
                 "addl $180, %%esp\n"
                 "jmp *%%eax\n"
                 :
                 : "m"(eip)
                 : "%eax");
}


int scheduler_replace(struct process *old, struct process *new)
{
    process_terminate(old);

    processes[old->pid] = new;

    return ALL_OK;
}

void handle_pit_interrupt(int interrupt, uint32_t unused)
{
    static uint32_t milliseconds = 0;

    pic_acknowledge();
    jiffies++;
    // if(jiffies % 60000 == 0) {
    //     kprintf(KCYN "\nOne minute has passed. Total uptime: %d seconds\n" KWHT, jiffies / 1000);
    // }
    milliseconds += PIT_INTERVAL;
    if (milliseconds >= TIME_SLICE) {
        milliseconds = 0;
        schedule();
    }
}

__attribute__((noreturn)) void scheduler_idle_task()
{
    asm("sti");
    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        // kprintf(KYEL "Idle!" KWHT);
        asm("hlt");
    }
}

void scheduler_initialize_idle_task()
{
    idle_task = kzalloc(sizeof(struct task));
    memset(idle_task, 0, sizeof(struct task));
    idle_task->tty          = 0;
    idle_task->registers.ip = (uint32_t)&scheduler_idle_task;
    idle_task->registers.cs = KERNEL_CODE_SELECTOR;
    // idle_task->registers.flags = 0x200; // Enable interrupts
    // idle_task->registers.ebp   = (uint32_t)kzalloc(4096) + 4096;
    // idle_task->registers.ss    = KERNEL_DATA_SELECTOR;
    // idle_task->registers.esp   = idle_task->registers.esp - 4;

    idle_task->process           = (struct process *)kzalloc(sizeof(struct process));
    idle_task->process->pid      = 0;
    idle_task->process->state    = RUNNING;
    idle_task->process->priority = 0;
    idle_task->process->task     = idle_task;
    strncpy(idle_task->process->file_name, "idle", 5);

    scheduler_set_process(idle_task->process->pid, idle_task->process);
}

int scheduler_init()
{
    scheduler_initialize_idle_task();
    pit_set_interval(PIT_INTERVAL);
    return idt_register_interrupt_callback(0x20, handle_pit_interrupt);
}

uint32_t scheduler_get_jiffies()
{
    return jiffies;
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
                .pid       = processes[i]->pid,
                .priority  = processes[i]->priority,
                .state     = processes[i]->state,
                .exit_code = processes[i]->exit_code,
            };
            strncpy(info.file_name, processes[i]->file_name, MAX_PATH_LENGTH);
            memcpy(*proc_info + i, &info, sizeof(struct process_info));
        }
    }

    return 0;
}

void scheduler_check_sleeping(struct process *process, struct task **next)
{
    if (process->signal == SIGWAKEUP && process->wait_pid != 0) {
        process->state  = WAITING;
        process->signal = NONE;
    } else if (process->signal == SIGWAKEUP) {
        process->state  = RUNNING;
        process->signal = NONE;
    } else if (process->sleep_until <= jiffies) {
        process->signal      = SIGWAKEUP;
        process->signal      = NONE;
        process->sleep_until = -1;
    } else {
        auto const runnable = scheduler_get_runnable_task();
        if (runnable) {
            *next = runnable;
        }
        if (runnable == nullptr) {
            scheduler_run_task_in_kernel_mode(idle_task->registers.ip);
        }
    }
}

/// @brief Gets the next task and runs it
void schedule()
{
    struct task *next = scheduler_get_next_task();

    if (!next) {
        // No tasks to run, restart the shell
        kprintf("\nRestarting the shell");
        scheduler_switch_to_any();
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

        process_terminate(next->process);
        return;
    }

    if (current_task->process->state == SLEEPING) {
        scheduler_check_sleeping(current_task->process, &next);
    }

    struct process *process = next->process;

    if (process->state == SLEEPING) {
        scheduler_check_sleeping(process, &next);
    }

    process = next->process;

    if (process->state == WAITING) {
        struct process *child;
        if (process->wait_pid == -1) {
            child = find_child_process_by_state(process, ZOMBIE);
        } else {
            child = find_child_process_by_pid(process, process->wait_pid);
            if (!child) {
                process->state     = RUNNING;
                process->exit_code = 0;
                process->wait_pid  = 0;
            }
        }

        if (child && child->state == ZOMBIE) {
            process->state     = RUNNING;
            process->exit_code = child->exit_code;
            process->wait_pid  = 0;

            if (child->state == TERMINATED || child->state == ZOMBIE) {
                process_remove_child(process, child);
                process_terminate(child);
            }
        } else if (child && child->state == RUNNING) {
            scheduler_switch_task(child->task);
            scheduler_run_task_in_user_mode(&child->task->registers);
        } else if (child && child->state == SLEEPING) {
            scheduler_check_sleeping(child, &next);
            if (child->state == RUNNING) {
                scheduler_switch_task(child->task);
                scheduler_run_task_in_user_mode(&child->task->registers);
            }
        } else {
            // Try to find another runnable task, if none, run the idle task
            next = scheduler_get_runnable_task();
            if (next == nullptr) {
                scheduler_run_task_in_kernel_mode(idle_task->registers.ip);
                return;
            }
        }
    }


    scheduler_switch_task(next);
    scheduler_run_task_in_user_mode(&next->registers);
}
