#pragma once

#include <config.h>
#include <process.h>

struct process_info {
    uint16_t pid;
    int priority;
    char file_name[MAX_PATH_LENGTH];
    enum PROCESS_STATE state;
    enum PROCESS_STATE wait_state;
    int exit_code;
};

__attribute__((nonnull)) struct process *scheduler_set_process(int pid, struct process *process);
void scheduler_switch_to_any(void);
struct process *scheduler_get_process(int pid);
struct process *scheduler_get_current_process(void);
__attribute__((nonnull)) void scheduler_unlink_process(const struct process *process);
int scheduler_get_free_pid(void);
__attribute__((nonnull)) void scheduler_unqueue_thread(struct thread *thread);
__attribute__((nonnull)) void scheduler_queue_thread(struct thread *thread);
__attribute__((nonnull)) void scheduler_add_to_zombie_list(struct process *thread);
__attribute__((nonnull)) void scheduler_remove_from_zombie_list(struct process *thread);
void schedule(void);
void scheduler_run_first_thread(void);
struct thread *scheduler_get_current_thread(void);
__attribute__((nonnull)) void scheduler_save_current_thread(const struct interrupt_frame *interrupt_frame);
__attribute__((nonnull)) int scheduler_switch_thread(struct thread *thread);

void set_user_mode_segments(void);
int scheduler_switch_current_thread_page(void);
__attribute__((nonnull)) int scheduler_replace(struct process *old, struct process *new);
uint32_t scheduler_get_jiffies(void);
void scheduler_init(void);
void scheduler_start(void);

__attribute__((nonnull)) int scheduler_get_processes(struct process_info **proc_info, int *count);
struct thread *scheduler_get_thread_sleeping_for_keyboard(void);
__attribute__((nonnull)) void scheduler_remove_current_thread(const struct thread *thread);
__attribute__((noreturn, naked)) void scheduler_idle_thread(void);
