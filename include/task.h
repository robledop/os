#pragma once

#include <config.h>
#include <list.h>
#include <paging.h>
#include <process.h>
#include <spinlock.h>
#include <stdint.h>

// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
// struct trap_frame {
//     // registers as pushed by pusha
//     uint32_t edi;
//     uint32_t esi;
//     uint32_t ebp;
//     uint32_t oesp; // useless & ignored
//     uint32_t ebx;
//     uint32_t edx;
//     uint32_t ecx;
//     uint32_t eax;
//
//     // rest of trap frame
//     uint32_t gs;
//     uint16_t fs;
//     uint16_t padding2;
//     uint16_t es;
//     uint16_t padding3;
//     uint16_t ds;
//     uint16_t padding4;
//     uint32_t trapno;
//
//     // below here defined by x86 hardware
//     uint32_t err;
//     uint32_t eip;
//     uint16_t cs;
//     uint16_t padding5;
//     uint32_t eflags;
//
//     // below here only when crossing rings, such as from user to kernel
//     uint32_t esp;
//     uint16_t ss;
//     uint16_t padding6;
// };

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;
};

enum task_state {
    TASK_RUNNING = 0,
    TASK_READY   = 1,
    TASK_SLEEPING,
    TASK_BLOCKED,
    TASK_STOPPED,
    TASK_PAUSED,
    TASK_STATE_COUNT,
};

enum task_alloc { ALLOC_STATIC, ALLOC_DYNAMIC };
enum task_mode { KERNEL_MODE, USER_MODE };
enum PROCESS_SIGNAL { NONE, SIGKILL, SIGSTOP, SIGCONT, SIGTERM, SIGWAKEUP };
enum SLEEP_REASON { SLEEP_REASON_NONE, SLEEP_REASON_STDIN };

struct task {
    int priority;
    enum task_state state;
    uint64_t time_used;
    struct interrupt_frame *trap_frame;
    struct task *next;
    uint64_t wakeup_time;
    void *wait_channel;
    const char *name;
    enum task_alloc alloc;
    struct context *context;
    struct page_directory *page_directory;
    int rand_id;
    char file_name[MAX_PATH_LENGTH];
    int wait_pid;
    int exit_code;
    uint32_t size;
    struct process *process;
    void *kernel_stack;
};

extern struct task *current_task;

#define TASK_ONLY if (current_task != NULL)
#define TIME_SLICE_SIZE (1 * 1000 * 1000ULL)

struct task_list {
    struct task *head;
    struct task *tail;
    spinlock_t lock;
};

#define MAX_TASKS_QUEUED 8
struct task_sync {
    struct task *possessor;
    const char *dbg_name;
    struct task_list waiting;
};

struct task *get_current_task(void);

static inline void tasks_sync_init(struct task_sync *ts)
{
    *ts = (struct task_sync){
        .possessor = nullptr,
        .dbg_name  = nullptr,
        .waiting   = {},
    };
}

/// @brief Initializes the kernel task manager.
void tasks_init();

void switch_context(struct context **, struct context *);
/// @brief Switches to a provided task.
/// @param task Pointer to the task struct
extern void tasks_switch_to(struct task *task);

/**
 * @brief Creates a new kernel task with a provided entry point, register storage struct,
 * and task state struct. If the storage parameter is provided, the struct task struct
 * provided will be written to. If NULL is passed as the storage parameter, a pointer
 * to a allocated task will be returned.
 *
 * @param entry Task function entry point
 * @param storage Task stack structure (if NULL, a pointer to the task is returned)
 * @param state Task state structure
 * @param name Task name (for debugging / printing)
 * @return struct task* Pointer to the created kernel task
 */
struct task *tasks_new(void (*entry)(void), struct task *storage, enum task_state state, const char *name,
                       enum task_mode mode);

/**
 * @brief Tell the kernel task scheduler to schedule all of the added tasks.
 *
 */
void tasks_schedule();

/**
 * @brief Returns the lifetime of the current task (in nanoseconds).
 *
 * @return uint64_t Task lifetime (in nanoseconds)
 */
uint64_t tasks_get_self_time();

/**
 * @brief Blocks the current task.
 *
 * @param reason
 */
void tasks_block_current(enum task_state reason);

/**
 * @brief Unblocks the current task.
 *
 * @param task
 */
void tasks_unblock(struct task *task);

/**
 * @brief Sleeps until the provided absolute time (in nanoseconds).
 *
 * @param time Absolute time to sleep until (in nanoseconds since boot)
 */
void tasks_nano_sleep_until(uint64_t time);

/**
 * @brief Sleep for a given period of time (in nanoseconds).
 *
 * @param time Nanoseconds to sleep
 */
void tasks_nano_sleep(uint64_t time);

/**
 * @brief Exits the current task.
 *
 */
void tasks_exit(void);

void tasks_sync_block(struct task_sync *ts);

void tasks_sync_unblock(struct task_sync *ts);

void *task_peek_stack_item(const struct task *task, int index);
void set_user_mode_segments(void);
int copy_string_from_task(const struct task *task, const void *virtual, void *physical, size_t max);

struct file *task_get_file_descriptor(const struct task *process, uint32_t index);
int task_new_file_descriptor(struct task *process, struct file **desc_out);
void task_free_file_descriptor(struct task *process, struct file *desc);
struct task *tasks_get_thread_sleeping_for_keyboard(void);

__attribute__((nonnull)) struct task *thread_create(struct process *process);
__attribute__((nonnull)) void *thread_virtual_to_physical_address(const struct task *task, void *virtual_address);
void tasks_set_idle_task(struct task *task);
void enqueue_task(struct task_list *list, struct task *task);
void current_task_page();
void sched(void);
void wakeup(struct task *task);
void tasks_remove_from_ready_queue(struct task *task);
void tasks_remove_from_sleeping_queue(struct task *task);
void tasks_remove_from_stopped_queue(struct task *task);
