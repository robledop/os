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
void scheduler_switch_to_any();
struct process *scheduler_get_process(const int pid);
struct process *scheduler_get_current_process();
void scheduler_unlink_process(const struct process *process);
int scheduler_get_free_pid();
void scheduler_unqueue_thread(struct thread *thread);
void scheduler_queue_thread(struct thread *thread);
void scheduler_add_to_zombie_list(struct process *thread);
void scheduler_remove_from_zombie_list(struct process *thread);
void schedule();
void scheduler_run_first_thread();
struct thread *scheduler_get_current_thread();
void scheduler_save_current_thread(const struct interrupt_frame *interrupt_frame);
int scheduler_switch_thread(struct thread *thread);

/// @brief Save registers and run the thread in user mode
void set_user_mode_segments();
int scheduler_switch_current_thread_page();
int scheduler_replace(struct process *old, struct process *new);
uint32_t scheduler_get_jiffies();
int scheduler_init();
void scheduler_run_idle_thread();


void scheduler_run_thread_in_kernel_mode(uint32_t eip);
int scheduler_get_processes(struct process_info **proc_info, int *count);
struct thread *scheduler_get_thread_sleeping_for_keyboard();
