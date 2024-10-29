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
#include <string.h>

// How often the PIT should interrupt
#define PIT_INTERVAL 1 // 1ms
// How often the scheduler should run
#define TIME_SLICE 10 // 10ms

// Milliseconds since boot
uint32_t jiffies                                = 0;
struct process *current_process                 = nullptr;
static struct process *processes[MAX_PROCESSES] = {nullptr};

struct thread *current_thread = nullptr;
struct thread *thread_tail    = nullptr;
struct thread *thread_head    = nullptr;
struct thread *idle_thread;
void scheduler_idle_thread();

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
            scheduler_switch_thread(current_process->thread);
            scheduler_run_thread_in_user_mode(&current_process->thread->registers);
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

struct thread *scheduler_get_current_thread()
{
    return current_thread;
}

int scheduler_get_thread_count()
{
    const struct thread *thread = thread_head;
    int count                 = 0;
    while (thread) {
        count++;
        thread = thread->next;
    }
    return count;
}

struct thread *scheduler_get_runnable_thread()
{
    struct thread *thread = thread_head;
    while (thread) {
        if (thread->process->state == RUNNING) {
            return thread;
        }
        thread = thread->next;
    }
    return nullptr;
}

struct thread *scheduler_get_next_thread()
{
    if (!current_thread && !thread_head) {
        return nullptr;
    }

    if (!current_thread) {
        return thread_head;
    }

    if (!current_thread->next) {
        return thread_head;
    }

    return current_thread->next;
}

void scheduler_queue_thread(struct thread *thread)
{
    if (!thread_head) {
        thread_head    = thread;
        thread_tail    = thread;
        current_thread = thread;
    } else {
        thread_tail->next = thread;
        thread->prev        = thread_tail;
        thread_tail       = thread;
    }
}

void scheduler_unqueue_thread(const struct thread *thread)
{
    if (thread->prev) {
        thread->prev->next = thread->next;
    }

    if (thread->next) {
        thread->next->prev = thread->prev;
    }

    if (thread == thread_head) {
        thread_head = thread->next;
    }

    if (thread == thread_tail) {
        thread_tail = thread->prev;
    }

    if (thread == current_thread) {
        current_thread = scheduler_get_next_thread();
    }
}


void scheduler_save_current_thread(const struct interrupt_frame *interrupt_frame)
{
    struct thread *thread = scheduler_get_current_thread();
    if (!thread) {
        panic("No current thread");
    }

    if (interrupt_frame->cs == KERNEL_CODE_SELECTOR) {
        thread_save_state(idle_thread, interrupt_frame);
    } else {
        thread_save_state(thread, interrupt_frame);
    }
}

/// @brief Set the thread as the current thread and switch to its page directory
/// @param thread the thread to run
/// @return ALL_OK on success, error code otherwise
int scheduler_switch_thread(struct thread *thread)
{
    current_thread = thread;
    set_user_mode_segments();
    paging_switch_directory(thread->process->page_directory);
    return ALL_OK;
}

/// @brief Set the user mode segments and switch to the current thread's page directory
int scheduler_switch_current_thread_page()
{
    set_user_mode_segments();
    paging_switch_directory(current_thread->process->page_directory);

    return ALL_OK;
}


void scheduler_run_thread_in_kernel_mode(uint32_t eip)
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

__attribute__((noreturn)) void scheduler_idle_thread()
{
    asm("sti");
    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        // kprintf(KYEL "Idle!" KWHT);
        asm("hlt");
    }
}

void scheduler_initialize_idle_thread()
{
    idle_thread = kzalloc(sizeof(struct thread));
    memset(idle_thread, 0, sizeof(struct thread));
    idle_thread->tty          = 0;
    idle_thread->registers.ip = (uint32_t)&scheduler_idle_thread;
    idle_thread->registers.cs = KERNEL_CODE_SELECTOR;

    idle_thread->process           = (struct process *)kzalloc(sizeof(struct process));
    idle_thread->process->pid      = 0;
    idle_thread->process->state    = RUNNING;
    idle_thread->process->priority = 0;
    idle_thread->process->thread   = idle_thread;
    strncpy(idle_thread->process->file_name, "idle", 5);

    scheduler_set_process(idle_thread->process->pid, idle_thread->process);
}

int scheduler_init()
{
    scheduler_initialize_idle_thread();
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

void scheduler_check_sleeping(struct process *process, struct thread **next)
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
        auto const runnable = scheduler_get_runnable_thread();
        if (runnable) {
            *next = runnable;
        }
        if (runnable == nullptr) {
            scheduler_run_thread_in_kernel_mode(idle_thread->registers.ip);
        }
    }
}

/// @brief Gets the next thread and runs it
void schedule()
{
    struct thread *next = scheduler_get_next_thread();

    if (!next) {
        // No threads to run, restart the shell
        kprintf("\nRestarting the shell");
        scheduler_switch_to_any();
        return;
    }

    // If the thread is terminated or a zombie, remove it from the scheduler
    if (next->process->state == TERMINATED || next->process->state == ZOMBIE) {
        // If the thread is a child process, signal the parent that the child has exited
        if (next->process->parent && next->process->parent->state == WAITING &&
            (next->process->parent->wait_pid == next->process->pid || next->process->parent->wait_pid == -1)) {
            next->process->parent->state     = RUNNING;
            next->process->parent->exit_code = next->process->exit_code;
            next->process->parent->wait_pid  = 0;
        }

        process_terminate(next->process);
        return;
    }

    if (current_thread->process->state == SLEEPING) {
        scheduler_check_sleeping(current_thread->process, &next);
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
            scheduler_switch_thread(child->thread);
            scheduler_run_thread_in_user_mode(&child->thread->registers);
        } else if (child && child->state == SLEEPING) {
            scheduler_check_sleeping(child, &next);
            if (child->state == RUNNING) {
                scheduler_switch_thread(child->thread);
                scheduler_run_thread_in_user_mode(&child->thread->registers);
            }
        } else {
            // Try to find another runnable thread, if none, run the idle thread
            next = scheduler_get_runnable_thread();
            if (next == nullptr) {
                scheduler_run_thread_in_kernel_mode(idle_thread->registers.ip);
                return;
            }
        }
    }


    scheduler_switch_thread(next);
    scheduler_run_thread_in_user_mode(&next->registers);
}
