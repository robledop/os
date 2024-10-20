#include <assert.h>
#include <config.h>
#include <idt.h>
#include <memory.h>
#include <process.h>
#include <scheduler.h>
#include <serial.h>
#include <status.h>

struct process *current_process                 = nullptr;
static struct process *processes[MAX_PROCESSES] = {nullptr};

struct task *current_task = nullptr;
struct task *task_tail    = nullptr;
struct task *task_head    = nullptr;

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
        if (processes[i] && processes[i]->state == RUNNING) {
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

void scheduler_run_first_task()
{
    if (!current_task) {
        panic("No tasks to run");
    }

    dbgprintf("Running first ever task %x\n", current_task);

    scheduler_switch_task(task_head);
    scheduler_run_task_in_user_mode(&task_head->registers);
}

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

/// @brief Gets the next task and runs it
void schedule()
{
    struct task *next = scheduler_get_next_task();
    if (!next) {
        // No tasks to run, restart the shell
        kprintf("\nRestarting the shell");
        start_shell(0);
        return;
    }

    auto const process = next->process;

    // TODO: Add a way for the child process to signal to the parent that it has exited
    if (process->state == WAITING) {
        ASSERT(process->wait_pid > 0, "A waiting process should have a wait pid");
        auto const child = find_child_process_by_pid(process, process->wait_pid);
        if (child->state == ZOMBIE) {
            process->state     = RUNNING;
            process->exit_code = child->exit_code;
            process->wait_pid  = 0;
            process_remove_child(process, child);
            scheduler_unlink_process(child);
        } else {
            // If the task is waiting and the child is still running, switch to the child task
            scheduler_switch_task(child->task);
            scheduler_run_task_in_user_mode(&child->task->registers);
        }
    }

    scheduler_switch_task(next);
    scheduler_run_task_in_user_mode(&next->registers);
}
