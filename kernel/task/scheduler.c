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
#define PIT_INTERVAL 100 // 1ms
// How often the scheduler should run
#define TIME_SLICE 100 // 100ms

// Milliseconds since boot
uint32_t jiffies                                = 0;
static struct process *processes[MAX_PROCESSES] = {nullptr};
spinlock_t scheduler_lock                       = 0;

struct list thread_list;
struct list zombie_list;
extern uint32_t stack_top;

struct thread *current_thread = nullptr;

void scheduler_initialize_idle_thread(uint32_t idle_thread_stack_address);


__attribute__((noreturn, naked)) void scheduler_idle_thread()
{
    // TODO
    // ! HACK: I'm resetting the stack pointer to the top manually every time the idle "thread" runs.
    // ! This is to prevent the stack pointer from drifting down the stack and eventually causing a stack overflow.
    // ! It seems to be drifting 240 bytes at a time.
    // ! This would happen even when I loaded this function in a proper separate thread with its
    // ! own stack (I probably didn't do that right either).
    // ! This will probably bite me later. I should figure out how to properly manage the kernel stack.
    asm volatile("mov %0, %%esp" ::"r"(stack_top));

    asm volatile("sti\n"
                 "_idle_loop:\n"
                 "hlt\n"
                 "jmp _idle_loop");
}

struct process *scheduler_get_current_process()
{
    auto const current_thread = scheduler_get_current_thread();
    if (current_thread) {
        return current_thread->process;
    }
    return nullptr;
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

void scheduler_remove_current_thread(const struct thread *thread)
{
    if (current_thread == thread) {
        current_thread = nullptr;
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

struct thread *scheduler_get_thread_sleeping_for_keyboard()
{
    const size_t size = list_size(&thread_list);
    if (size == 0) {
        return nullptr;
    }

    auto thread = list_entry(list_begin(&thread_list), struct thread, elem);
    for (size_t i = 0; i < size; i++) {
        if (thread->process->state == SLEEPING && thread->process->sleep_reason == SLEEP_REASON_KEYBOARD) {
            return thread;
        }
        thread = list_entry(list_next(&thread->elem), struct thread, elem);
    }

    return nullptr;
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
    if (list_size(&thread_list) == 0) {
        return nullptr;
    }

    return list_entry(list_begin(&thread_list), struct thread, elem);
}

void scheduler_queue_thread(struct thread *thread)
{
    list_push_back(&thread_list, &thread->elem);
}


void scheduler_remove_from_zombie_list(struct process *process)
{
    list_remove(&process->elem);
}

void scheduler_unqueue_thread(struct thread *thread)
{
    list_remove(&thread->elem);
}

void scheduler_save_current_thread(const struct interrupt_frame *interrupt_frame)
{
    if (interrupt_frame->cs == USER_CODE_SELECTOR) {
        struct thread *thread = scheduler_get_current_thread();
        if (!thread) {
            kprintf(KMAG "." KWHT);
            return;
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

    current_thread = thread;
    paging_switch_directory(thread->process->page_directory);
    thread_switch(&thread->registers);
    return ALL_OK;
}

/// @brief Set the user mode segments and switch to the current thread's page directory
int scheduler_switch_current_thread_page()
{
    auto const process = scheduler_get_current_process();
    if (!process) {
        return -EINVARG;
    }

    ASSERT(process->state != ZOMBIE, "Trying to switch to a zombie thread");

    if (process->thread->registers.cs == KERNEL_CODE_SELECTOR) {
        set_kernel_mode_segments();
    } else if (process->thread->registers.cs == USER_CODE_SELECTOR) {
        set_user_mode_segments();
    } else {
        panic("Unknown code selector");
    }

    paging_switch_directory(scheduler_get_current_process()->page_directory);

    return ALL_OK;
}


int scheduler_replace(struct process *old, struct process *new)
{
    process_zombify(old);

    processes[old->pid] = new;

    return ALL_OK;
}

void handle_pit_interrupt(int interrupt, const struct interrupt_frame *frame)
{
    static uint32_t milliseconds = 0;

    jiffies += PIT_INTERVAL;

    milliseconds += PIT_INTERVAL;
    // asm volatile("mov %0, %%esp\n" : : "r"(frame->reserved));
    if (milliseconds >= TIME_SLICE) {
        milliseconds = 0;
        cli();
        pic_acknowledge(interrupt);
        // asm volatile("mov %0, %%esp\n" : : "r"(frame->reserved));
        schedule();
    }
}

int scheduler_init()
{
    list_init(&thread_list);
    list_init(&zombie_list);

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

void scheduler_check_waiting_thread(const struct thread *next)
{
    auto const process = next->process;

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

            process_remove_child(process, child);

            kfree(child);
        }
    }
}

/// Move the first thread in the thread list to the end of the queue
void scheduler_rotate_queue()
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

    scheduler_rotate_queue();

    auto next = scheduler_get_next_thread();

    if (!next) {
        kprintf("\nRestarting the shell");
        start_shell(0);
        next = scheduler_get_next_thread();
    }

    ASSERT(thread_is_valid(next));

    struct process *process = next->process;

    // If the next thread is sleeping, check if it should wake up.
    if (process->state == SLEEPING) {
        scheduler_check_sleeping(process);
    }

    // If the next thread is waiting, check their children
    // and remove any zombies.
    scheduler_check_waiting_thread(next);

    // If the next thread is not running, find a runnable thread.
    // If no runnable thread is found, run the idle thread.
    if (next->process->state != RUNNING) {
        next = scheduler_get_runnable_thread();
        if (!next) {
            scheduler_idle_thread();
            return;
        } else {
            // If we found a runnable thread, put it in the front of the queue
            list_remove(&next->elem);
            list_push_front(&thread_list, &next->elem);
        }
    }

    ASSERT(next->process->page_directory);
    // If the next thread is in the RUNNING state, switch to it.
    scheduler_switch_thread(next);
}
