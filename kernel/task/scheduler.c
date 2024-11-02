#include <config.h>
#include <debug.h>
#include <idt.h>
#include <kernel_heap.h>
#include <list.h>
#include <memory.h>
#include <pic.h>
#include <pit.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <string.h>
#include <x86.h>

// How often the PIT should interrupt
#define PIT_INTERVAL 1 // 1ms
// How often the scheduler should run
#define TIME_SLICE 100 // 100ms

// Milliseconds since boot
uint32_t jiffies                                = 0;
static struct process *processes[MAX_PROCESSES] = {nullptr};
spinlock_t scheduler_lock                       = 0;

struct list thread_list;
struct list zombie_list;

// struct thread *current_thread = nullptr;
// struct thread *thread_tail    = nullptr;
// struct thread *thread_head    = nullptr;

struct thread *idle_thread;
void scheduler_idle_thread();
void scheduler_initialize_idle_thread();

struct process *scheduler_get_current_process()
{
    return scheduler_get_current_thread()->process;
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

bool scheduler_is_shell_running()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i] && strncmp(processes[i]->file_name, "0:/bin/sh", strlen("0:/bin/sh")) == 0) {
            return true;
        }
    }

    return false;
}

void scheduler_unlink_process(const struct process *process)
{
    processes[process->pid] = nullptr;
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

/// The current thread is the first thread in the thread list
struct thread *scheduler_get_current_thread()
{
    if (list_empty(&thread_list)) {
        kprintf("Restarting the shell\n");
        start_shell(0);
    }
    return list_entry(list_front(&thread_list), struct thread, elem);
}

struct thread *scheduler_get_runnable_thread()
{
    const size_t size = list_size(&thread_list);
    if (size == 0) {
        return nullptr;
    }

    auto thread = list_entry(list_begin(&thread_list), struct thread, elem);
    for (size_t i = 0; i < size; i++) {
        if (thread->process->state == RUNNING) {
            return thread;
        }
        thread = list_entry(list_next(&thread->elem), struct thread, elem);
    }

    return nullptr;
}

struct thread *scheduler_get_next_thread()
{
    return list_entry(list_begin(&thread_list), struct thread, elem);
}

void scheduler_queue_thread(struct thread *thread)
{
    list_push_back(&thread_list, &thread->elem);
}

void scheduler_add_to_zombie_list(struct process *process)
{
    list_push_back(&zombie_list, &process->elem);
}

void scheduler_remove_from_zombie_list(struct process *process)
{
    list_remove(&process->elem);
}

void scheduler_unqueue_thread(struct thread *thread)
{
    list_remove(&thread->elem);
}

struct process *scheduler_find_zombie_child(const struct process *parent)
{
    struct process *process = nullptr;
    for (size_t i = 0; i < list_size(&zombie_list); i++) {
        process = list_entry(list_begin(&zombie_list), struct process, elem);
        if (process->parent == parent) {
            return process;
        }
    }

    return nullptr;
}

struct process *scheduler_find_zombie_child_by_pid(const struct process *parent, const int pid)
{
    struct process *process = nullptr;
    for (size_t i = 0; i < list_size(&zombie_list); i++) {
        process = list_entry(list_begin(&zombie_list), struct process, elem);
        if (process->parent == parent && process->pid == pid) {
            return process;
        }
    }

    return nullptr;
}

void scheduler_save_current_thread(const struct interrupt_frame *interrupt_frame)
{
    if (interrupt_frame->cs == KERNEL_CODE_SELECTOR) {
        // thread_save_state(idle_thread, interrupt_frame);
    } else {
       struct thread *thread = scheduler_get_current_thread();
        if (!thread) {
            panic("No current thread");
        }
        thread_save_state(thread, interrupt_frame);
    }
}

/// @brief Set the thread as the current thread, switch to its page directory, and run it
/// @param thread the thread to run
/// @return ALL_OK on success, error code otherwise
int scheduler_switch_thread(struct thread *thread)
{
    ASSERT(thread->process->state != ZOMBIE, "Trying to switch to a zombie thread");
    ASSERT(thread_is_valid(thread));

    // current_thread = thread;
    paging_switch_directory(thread->process->page_directory);
    leave_critical();
    thread_switch(&thread->registers);
    return ALL_OK;
}

/// @brief Set the user mode segments and switch to the current thread's page directory
int scheduler_switch_current_thread_page()
{
    ASSERT(scheduler_get_current_process()->state != ZOMBIE, "Trying to switch to a zombie thread");

    set_user_mode_segments();
    paging_switch_directory(scheduler_get_current_process()->page_directory);

    return ALL_OK;
}

// void scheduler_run_thread_in_kernel_mode(uint32_t eip)
// {
//
//     asm volatile("mov %0, %%eax\n"
//                  "sti\n"
//                  "addl $180, %%esp\n"
//                  "jmp *%%eax\n"
//                  :
//                  : "m"(eip)
//                  : "%eax");
// }

void scheduler_run_idle_thread()
{
    kernel_page();
    leave_critical();
    ASSERT(read_eflags() & EFLAGS_IF, "Interrupts must be enabled");

    scheduler_switch_thread(idle_thread);
}


int scheduler_replace(struct process *old, struct process *new)
{
    process_zombify(old);

    processes[old->pid] = new;

    return ALL_OK;
}

void handle_pit_interrupt(int interrupt, uint32_t unused, const struct interrupt_frame *frame)
{
    static uint32_t milliseconds = 0;

    jiffies++;

    milliseconds += PIT_INTERVAL;
    if (milliseconds >= TIME_SLICE) {
        milliseconds = 0;
        pic_acknowledge(interrupt);
        enter_critical();
        schedule();
    }
}

__attribute__((noreturn)) void scheduler_idle_thread()
{
    kernel_page();
    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        asm volatile("hlt");
    }
}

void scheduler_initialize_idle_thread()
{
    // idle_thread                = kzalloc(sizeof(struct thread));
    // idle_thread->tty           = 0;
    // idle_thread->registers.ip  = (uint32_t)&scheduler_idle_thread;
    // idle_thread->registers.cs  = KERNEL_CODE_SELECTOR;
    // idle_thread->registers.ss  = KERNEL_DATA_SELECTOR;
    // idle_thread->registers.esp = (uint32_t)idle_thread + sizeof(struct thread);

    struct process *process = (struct process *)kzalloc(sizeof(struct process));
    process->pid            = 0;
    process->state          = RUNNING;
    process->priority       = 0;
    process->file_type      = PROCESS_FILE_TYPE_BINARY;

    idle_thread                  = thread_create(process);
    idle_thread->registers.ip    = (uint32_t)&scheduler_idle_thread;
    idle_thread->registers.cs    = KERNEL_CODE_SELECTOR;
    idle_thread->registers.ss    = KERNEL_DATA_SELECTOR;
    idle_thread->process         = process;
    idle_thread->registers.flags = EFLAGS_IF;


    // idle_thread->process->thread   = idle_thread;
    strncpy(idle_thread->process->file_name, "idle", 5);

    scheduler_set_process(idle_thread->process->pid, idle_thread->process);
}

int scheduler_init()
{
    list_init(&thread_list);
    list_init(&zombie_list);

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

void scheduler_check_sleeping(struct process *process)
{
    if (process->signal == SIGWAKEUP && process->wait_pid != 0) {
        process->state  = WAITING;
        process->signal = NONE;
    } else if (process->signal == SIGWAKEUP && (int)process->sleep_until == -1) {
        process->state  = RUNNING;
        process->signal = NONE;
    } else if (process->sleep_until <= jiffies) {
        process->signal      = SIGWAKEUP;
        process->sleep_until = -1;
    }
}

void scheduler_check_zombie_child_thread(const struct thread *next)
{
    if (next->process->state == ZOMBIE) {
        // If the thread is a child process, signal the parent that the child has exited
        if (next->process->parent && next->process->parent->state == WAITING &&
            (next->process->parent->wait_pid == next->process->pid || next->process->parent->wait_pid == -1)) {
            next->process->parent->state     = RUNNING;
            next->process->parent->exit_code = next->process->exit_code;
            next->process->parent->wait_pid  = 0;
        }
    }
}

void scheduler_check_waiting_thread(const struct thread *next)
{
    auto const process = next->process;

    if (process->state == WAITING) {
        struct process *child;
        if (process->wait_pid == -1) {
            // child = find_child_process_by_state(process, ZOMBIE);
            child = scheduler_find_zombie_child(process);
        } else {
            child = find_child_process_by_pid(process, process->wait_pid);
            if (!child) {
                child = scheduler_find_zombie_child_by_pid(process, process->wait_pid);
            }
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

            process_remove_child(process, child);

            kfree(child);
        }
    }
}

/// Move the first thread in the thread list to the end of the queue
void scheduler_rotate()
{
    if (list_size(&thread_list) > 1) {
        enter_critical();
        auto const current_thread = list_entry(list_pop_front(&thread_list), struct thread, elem);
        if (current_thread) {
            list_push_back(&thread_list, &current_thread->elem);
        }
        leave_critical();
    }
}

/// @brief Gets the next thread and runs it
/// @remark Must be called with interrupts disabled
void schedule()
{
    ASSERT(!(read_eflags() & EFLAGS_IF), "Interrupts must be disabled");

    scheduler_rotate();

    auto next = scheduler_get_next_thread();

    if (!next) {
        kprintf("Restarting the shell\n");
        start_shell(0);
        leave_critical();
        return;
    }

    // If the next thread is a zombie, and its parent is also a zombie (or non-existent), free the parent and the child
    if (next->process->state == ZOMBIE &&
        ((next->process->parent && next->process->parent->state == ZOMBIE) || next->process->parent == nullptr)) {
        if (next->process->parent) {
            kfree(next->process->parent);
            next->process->parent = nullptr;
        }
        kfree(next);
        scheduler_rotate();
        next = scheduler_get_next_thread();
    }

    // If the next thread is a zombie, get its exit code, give it to the parent, and then find the next thread
    scheduler_check_zombie_child_thread(next);
    if (next->process->state == ZOMBIE) {
        scheduler_rotate();
        next = scheduler_get_next_thread();
    }

    struct process *process = next->process;

    // If the next thread is sleeping, check if it should wake up.
    if (process->state == SLEEPING) {
        scheduler_check_sleeping(process);
    }

    // If the next thread is waiting, check their children
    scheduler_check_waiting_thread(next);

    // If the next thread is not running, find a runnable thread
    // If no runnable thread is found, run the idle thread
    if (next->process->state != RUNNING) {
        next = scheduler_get_runnable_thread();
        if (!next) {
            scheduler_run_idle_thread();
            return;
        } else {
            // If we found a runnable thread, put it in the front of the queue
            list_remove(&next->elem);
            list_push_front(&thread_list, &next->elem);
        }
    }

    ASSERT(next->process->page_directory);
    // If the next thread is in the RUNNING state, switch to it
    scheduler_switch_thread(next);
}
