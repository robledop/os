#pragma once

#include "config.h"
#include "process.h"


struct process_info {
    uint16_t pid;
    int priority;
    char file_name[MAX_PATH_LENGTH];
    enum PROCESS_STATE state;
    enum PROCESS_STATE wait_state;
    int exit_code;
};

struct process *scheduler_set_process(const int pid, struct process *process);
void scheduler_set_current_process(struct process *process);
void scheduler_switch_to_any();
struct process *scheduler_get_process(const int pid);
struct process *scheduler_get_current_process();
void scheduler_unlink_process(const struct process *process);
int scheduler_get_free_pid();
void scheduler_unqueue_task(const struct task *task);
void scheduler_queue_task(struct task *task);
void schedule();
void scheduler_run_first_task();
struct task *scheduler_get_current_task();
void scheduler_save_current_task(const struct interrupt_frame *interrupt_frame);
int scheduler_switch_task(struct task *task);

/// @brief Save registers and run the task in user mode
void scheduler_run_task_in_user_mode(struct registers *registers);
void set_user_mode_segments();
int scheduler_switch_current_task_page();
int scheduler_replace(struct process *old, struct process *new);
uint32_t scheduler_get_jiffies();
int scheduler_init();


int scheduler_get_processes(struct process_info **proc_info, int *count);
