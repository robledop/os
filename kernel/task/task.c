#include <config.h>
#include <elf.h>
#include <idt.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memory.h>
#include <paging.h>
#include <printf.h>
#include <process.h>
#include <serial.h>
#include <status.h>
#include <string.h>
#include <task.h>
#include <termcolors.h>
#include <timer.h>
#include <tss.h>
#include <x86.h>
#include <x86gprintrin.h>

extern void trap_return(void);

extern struct tss_entry tss_entry;
struct context *scheduler_context;

extern struct page_directory *kernel_page_directory;

static struct task *dequeue_task(struct task_list *);
static void cleaner_task_impl(void);
static void schedule(void);
void tasks_enqueue_ready(struct task *task);
void tasks_update_time();

/* macro to create a new named task_list and associated helper functions */
#define TASK_QUEUE(name)                                                                                               \
    struct task_list name##_queue = {/* Zero */};                                                                      \
    static inline void enqueue_##name(struct task *task)                                                               \
    {                                                                                                                  \
        enqueue_task(&name##_queue, task);                                                                             \
    }                                                                                                                  \
    static inline struct task *dequeue_##name()                                                                        \
    {                                                                                                                  \
        return dequeue_task(&name##_queue);                                                                            \
    }

struct task *current_task = nullptr;
struct task *idle_task    = nullptr;
static struct task cleaner_task;

TASK_QUEUE(ready)
TASK_QUEUE(sleeping)
TASK_QUEUE(stopped)

// map between task state and the list it is in
static struct task_list *task_queues[TASK_STATE_COUNT] = {
    [TASK_RUNNING]  = nullptr, // not in a list
    [TASK_READY]    = &ready_queue,
    [TASK_SLEEPING] = &sleeping_queue,
    [TASK_BLOCKED]  = nullptr, // in a list specific to the blocking primitive
    [TASK_STOPPED]  = &stopped_queue,
    [TASK_PAUSED]   = nullptr, // not in a list
};

// map between task state and its name
static const char *state_names[TASK_STATE_COUNT] = {
    [TASK_RUNNING]  = "RUNNING",
    [TASK_READY]    = "READY",
    [TASK_SLEEPING] = "SLEEPING",
    [TASK_BLOCKED]  = "BLOCKED",
    [TASK_STOPPED]  = "STOPPED",
    [TASK_PAUSED]   = "PAUSED",
};

static uint64_t idle_time              = 0;
static uint64_t idle_start             = 0;
static uint64_t last_time              = 0;
static uint64_t time_slice_remaining   = 0;
static uint64_t last_timer_time        = 0;
static size_t scheduler_lock           = 0;
static size_t scheduler_postpone_count = 0;
static bool scheduler_postponed        = false;
static uint64_t instr_per_ns;

void sched(void)
{
    switch_context(&current_task->context, scheduler_context);
}

static void acquire_scheduler_lock()
{
    asm volatile("cli");
    scheduler_postpone_count++;
    scheduler_lock++;
}

static void release_scheduler_lock()
{
    scheduler_postpone_count--;
    if (scheduler_postpone_count == 0) {
        if (scheduler_postponed) {
            scheduler_postponed = false;
            schedule();
        }
    }
    scheduler_lock--;
    if (scheduler_lock == 0) {
        asm volatile("sti");
    }
}

struct task *get_current_task(void)
{
    return current_task;
}

static void discover_cpu_speed()
{
    sti();
    const uint32_t curr_tick = timer_tick;
    uint64_t curr_rtsc       = __rdtsc();
    while (timer_tick != curr_tick + 1) {
        // wait for the next tick
    }
    curr_rtsc    = __rdtsc() - curr_rtsc;
    instr_per_ns = curr_rtsc / 1000000;
    if (instr_per_ns == 0) {
        instr_per_ns = 1;
    }
    cli();
}

static inline uint64_t get_cpu_time_ns()
{
    return (__rdtsc()) / instr_per_ns;
}

static void on_timer();

void tasks_init()
{
    discover_cpu_speed();
    (void)tasks_new(cleaner_task_impl, &cleaner_task, TASK_PAUSED, "[cleaner]", KERNEL_MODE);
    cleaner_task.state   = TASK_PAUSED;
    last_time            = get_cpu_time_ns();
    last_timer_time      = last_time;
    time_slice_remaining = TIME_SLICE_SIZE;
    timer_register_callback(on_timer);
}

static void task_starting()
{
    // this is called whenever a new task is about to start
    // it is run in the context of the new task

    // the task before this caused the scheduler to lock
    // so we must unlock here
    scheduler_lock--;
    if (scheduler_lock == 0) {
        asm volatile("sti");
    }
}

static void task_stopping()
{
    tasks_exit();
    panic("Attempted to schedule a dead task");
}

static inline void stack_push_pointer(char **stack_pointer, const uintptr_t value)
{
    *(uintptr_t *)stack_pointer -= sizeof(uintptr_t); // make room for a pointer
    **(uintptr_t **)stack_pointer = value;            // push the pointer onto the stack
}

void enqueue_task(struct task_list *list, struct task *task)
{
    spin_lock(&list->lock);

    if (list->head == NULL) {
        list->head = task;
    }
    if (list->tail != NULL) {
        // the current last task's next pointer will be this task
        list->tail->next = task;
    }
    // and now this task becomes the last task
    task->next = nullptr;
    list->tail = task;

    spin_unlock(&list->lock);
}

static struct task *dequeue_task(struct task_list *list)
{
    if (list->head == NULL) {
        // can't dequeue if there's not anything there
        return nullptr;
    }
    // the head of the list is the next item
    struct task *task = list->head;
    // the new head is the next task
    list->head = task->next;
    // so null its previous pointer
    if (list->head == NULL) {
        // if there are no more items in the list, then
        // the last item in the list will also be null
        list->tail = nullptr;
    }
    // it doesn't make sense to have a next when it's not in a list
    task->next = nullptr;
    return task;
}

static void remove_task(struct task_list *list, struct task *task, struct task *previous)
{
    // if this is true, something's not right...
    if (previous != NULL && previous->next != task) {
        panic("Bogus arguments to remove_task.");
    }
    // update the head if necessary
    if (list->head == task) {
        list->head = task->next;
    }
    // update the tail if necessary
    if (list->tail == task) {
        list->tail = previous;
    }
    // update the previous task if necessary
    if (previous != NULL) {
        previous->next = task->next;
    }
    // it's not in any list anymore, so clear its next pointer
    task->next = nullptr;
}

extern void tasks_enqueue_ready(struct task *task)
{
    enqueue_task(&ready_queue, task);
}

static struct task *task_state_dequeue_ready()
{
    return dequeue_task(&ready_queue);
}

struct task *tasks_new(void (*entry)(void), struct task *storage, const enum task_state state, const char *name,
                       enum task_mode mode)
{
    struct task *new_task = storage;
    if (storage == nullptr) {
        new_task = (struct task *)kzalloc(sizeof(struct task));
    }
    if (new_task == NULL) {
        panic("Unable to allocate memory for new task struct.");
        return nullptr;
    }

    // ReSharper disable once CppDFAMemoryLeak
    uint8_t *kernel_stack = kzalloc(KERNEL_STACK_SIZE);
    if (kernel_stack == nullptr) {
        panic("Unable to allocate memory for new task stack.");
        return nullptr;
    }

    new_task->kernel_stack = kernel_stack;

    auto kernel_stack_pointer = (char *)(kernel_stack + KERNEL_STACK_SIZE);
    stack_push_pointer(&kernel_stack_pointer, (uintptr_t)task_stopping);

    if (mode == USER_MODE) {
        // When trap_return is called, it will use the trap frame to restore the state of the task
        // so we later need to change cs, ds, es, ss, eflags, esp, and eip to point to the program we want to run
        kernel_stack_pointer -= sizeof(*new_task->trap_frame);
        new_task->trap_frame = (struct interrupt_frame *)kernel_stack_pointer;

        new_task->trap_frame->cs     = USER_CODE_SELECTOR;
        new_task->trap_frame->ds     = USER_DATA_SELECTOR;
        new_task->trap_frame->es     = USER_DATA_SELECTOR;
        new_task->trap_frame->ss     = USER_DATA_SELECTOR;
        new_task->trap_frame->eflags = EFLAGS_IF;
        new_task->trap_frame->esp    = USER_STACK_TOP;
        new_task->trap_frame->eip    = PROGRAM_VIRTUAL_ADDRESS;

        stack_push_pointer(&kernel_stack_pointer, (uintptr_t)trap_return);
    } else if (mode == KERNEL_MODE) {
        stack_push_pointer(&kernel_stack_pointer, (size_t)entry);

        struct page_directory *page_directory = paging_create_directory(
            PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);

        new_task->page_directory = page_directory;
    }

    kernel_stack_pointer -= sizeof *new_task->context;
    new_task->context = (struct context *)kernel_stack_pointer;
    memset(new_task->context, 0, sizeof *new_task->context);
    new_task->context->eip = (uintptr_t)task_starting;

    new_task->next      = nullptr;
    new_task->state     = state;
    new_task->time_used = 0;
    new_task->name      = name;
    new_task->alloc     = storage == NULL ? ALLOC_DYNAMIC : ALLOC_STATIC;
    if (state == TASK_READY) {
        tasks_enqueue_ready(new_task);
    }
    // ReSharper disable once CppDFAMemoryLeak
    return new_task;
}

void tasks_update_time()
{
    const uint64_t current_time = get_cpu_time_ns();
    const uint64_t delta        = current_time - last_time;
    if (current_task == NULL) {
        idle_time += delta;
    } else {
        current_task->time_used += delta;
    }
    last_time = current_time;
}

void tasks_switch_to(struct task *task)
{
    if (current_task->state == TASK_RUNNING && current_task != idle_task) {
        current_task->state = TASK_READY;
        tasks_enqueue_ready(current_task);
    }

    write_tss(5, 0x10, (uintptr_t)task->kernel_stack + KERNEL_STACK_SIZE);

    paging_switch_directory(task->page_directory);

    task->state  = TASK_RUNNING;
    current_task = task;
    switch_context(&scheduler_context, task->context);

    // paging_switch_directory(kernel_page_directory);
}

static void schedule()
{
    if (scheduler_postpone_count != 0) {
        // don't schedule if there's more work to be done
        scheduler_postponed = true;
        return;
    }

    // ReSharper disable once CppDFAEndlessLoop
    while (1) {
        struct task *task = task_state_dequeue_ready();

        tasks_update_time();
        time_slice_remaining = TIME_SLICE_SIZE;
        last_timer_time      = get_cpu_time_ns();

        if (task == nullptr) {
            tasks_switch_to(idle_task);
        } else {
            tasks_switch_to(task);
        }
    }
}

void tasks_set_idle_task(struct task *task)
{
    idle_task = task;
}

void tasks_schedule()
{
    acquire_scheduler_lock();
    schedule();

    // this will run when we switch back to the calling task
    release_scheduler_lock();
}

uint64_t tasks_get_self_time()
{
    tasks_update_time();
    return current_task->time_used;
}

void tasks_block_current(const enum task_state reason)
{
    acquire_scheduler_lock();
    current_task->state = reason;
    schedule();
    release_scheduler_lock();
}

void tasks_unblock(struct task *task)
{
    acquire_scheduler_lock();
    task->state = TASK_READY;
    tasks_enqueue_ready(task);
    release_scheduler_lock();
}

void wakeup(struct task *task)
{
    task->state       = TASK_READY;
    task->wakeup_time = (0ULL - 1);
    tasks_enqueue_ready(task);
}


void tasks_remove_from_queue(struct task_list *list, const struct task *task)
{
    struct task *pre          = nullptr;
    struct task *task_pointer = list->head;

    while (task_pointer != NULL) {
        if (task_pointer == task) {
            remove_task(list, task_pointer, pre);
            task_pointer->next = nullptr;
            break;
        }
        struct task *next = task_pointer->next;
        pre               = task_pointer;
        task_pointer      = next;
    }
}

void tasks_remove_from_ready_queue(struct task *task)
{
    // tasks_remove_from_queue(&ready_queue, task);
}

void tasks_remove_from_sleeping_queue(struct task *task)
{
    // tasks_remove_from_queue(&sleeping_queue, task);
}

void tasks_remove_from_stopped_queue(struct task *task)
{
    // tasks_remove_from_queue(&stopped_queue, task);
}

static void on_timer()
{
    acquire_scheduler_lock();

    struct task *pre    = nullptr;
    struct task *task   = sleeping_queue.head;
    bool need_schedule  = false;
    const uint64_t time = get_cpu_time_ns();

    while (task != NULL) {
        struct task *next = task->next;
        if (time >= task->wakeup_time) {
            printf(KWHT "timer: waking sleeping task\n");
            remove_task(&sleeping_queue, task, pre);
            wakeup(task);
            task->next    = nullptr;
            need_schedule = true;
        } else {
            pre = task;
        }
        task = next;
    }

    if (time_slice_remaining != 0) {
        const uint64_t time_delta = time - last_timer_time;
        last_timer_time           = time;
        if (time_delta >= time_slice_remaining) {
            // schedule (and maybe pre-empt)
            // the schedule function will reset the time slice
            // printf(KWHT "timer: time slice expired\n");
            need_schedule = true;
        } else {
            // decrement the time slice counter
            time_slice_remaining -= time_delta;
        }
    }

    if (need_schedule) {
        if (scheduler_context != nullptr) {
            sched();
        } else {
            schedule();
        }
    }

    release_scheduler_lock();
}

void tasks_nano_sleep_until(const uint64_t time)
{
    acquire_scheduler_lock();
    current_task->state       = TASK_SLEEPING;
    current_task->wakeup_time = time;
    enqueue_sleeping(current_task);
    sched();
    release_scheduler_lock();
}

void tasks_nano_sleep(const uint64_t time)
{
    tasks_nano_sleep_until(get_cpu_time_ns() + time);
}

void tasks_exit()
{
    cli();
    // userspace cleanup can happen here
    // printf(KWHT "task \"%s\" (0x%08lx) exiting\n", current_task->name, (uint32_t)current_task);

    acquire_scheduler_lock();
    // all scheduling-specific operations must happen here
    enqueue_stopped(current_task);

    // the ordering of these two should really be reversed
    // but the scheduler currently isn't very smart
    tasks_block_current(TASK_STOPPED);

    if (current_task->process->parent) {
        process_wakeup(current_task->process->parent);
    }

    tasks_unblock(&cleaner_task);
    sched();

    release_scheduler_lock();
}

static void clean_stopped_task(struct task *task)
{
    kfree(&task->kernel_stack);
    // somehow determine if the task was dynamically allocated or not
    // just assume statically allocated tasks will never exit (bad idea)
    if (task->alloc == ALLOC_DYNAMIC) {
        kfree(task);
    }
}

static void cleaner_task_impl()
{
    // ReSharper disable once CppDFAEndlessLoop
    for (;;) {
        // acquire_scheduler_lock();
        cli();

        while (stopped_queue.head != NULL) {
            struct task *task = dequeue_stopped();
            process_set(task->process->pid, nullptr);
            clean_stopped_task(task);
        }

        // a schedule occurring at this point would be okay
        // it just needs to occur before the loop repeats
        tasks_block_current(TASK_PAUSED);

        // release_scheduler_lock();
    }
}

void tasks_sync_block(struct task_sync *ts)
{
    if (!current_task) {
        return;
    }
    acquire_scheduler_lock();
    // push the current task to the waiting queue
    enqueue_task(&ts->waiting, current_task);
    // now block until the mutex is freed
    tasks_block_current(TASK_BLOCKED);
    release_scheduler_lock();
}

void tasks_sync_unblock(struct task_sync *ts)
{
    acquire_scheduler_lock();
    // iterate all tasks that were blocked and unblock them
    struct task *task = ts->waiting.head;
    struct task *next = nullptr;
    if (task == nullptr) {
        // no other tasks were blocked
        goto exit;
    }
    do {
        next = task->next;
        wakeup(task);
        task->next = nullptr;
        task       = next;
    } while (task != nullptr);
    ts->waiting.head = nullptr;
    ts->waiting.tail = nullptr;
    // we woke up some tasks
    schedule();
exit:
    release_scheduler_lock();
}

void tasks_switch_page_directory(const struct task *task)
{
    // set_user_mode_segments();
    paging_switch_directory(task->page_directory);
}

void *task_peek_stack_item(const struct task *task, const int index)
{
    const uint32_t *stack_pointer = (uint32_t *)task->trap_frame->esp;
    current_task_page();
    auto const result = (void *)stack_pointer[index];
    kernel_page();
    return result;
}


int copy_string_from_task(const struct task *task, const void *virtual, void *physical, const size_t max)
{
    int res   = 0;
    char *tmp = process_malloc(task->process, max);
    if (!tmp) {
        dbgprintf("Failed to allocate memory for string\n");
        res = -ENOMEM;
        goto out;
    }

    const uint32_t old_entry = paging_get(task->page_directory, tmp);

    paging_map(task->page_directory,
               tmp,
               tmp,
               PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
                   PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    paging_switch_directory(task->page_directory);
    strncpy(tmp, virtual, max);
    kernel_page();
    res = paging_set(task->page_directory, tmp, old_entry);
    if (res < 0) {
        dbgprintf("Failed to set page\n");
        res = -EIO;
        goto out_free;
    }

    strncpy(physical, tmp, max);

out_free:
    kfree(tmp);
out:
    return res;
}

// void user_init(void)
// {
//     extern char _binary___build_apps_blank_start[], _binary___build_apps_blank_size[]; //
//     NOLINT(*-reserved-identifier)
//
//     char *program_start = _binary___build_apps_blank_start;
//     size_t program_size = (size_t)_binary___build_apps_blank_size;
//
//     struct task *new_task = tasks_new(nullptr, nullptr, TASK_PAUSED, "init", USER_MODE);
//     void *program         = kzalloc(program_size);
//     memcpy(program, program_start, program_size);
//     paging_map_to(new_task->page_directory,
//                   (void *)PROGRAM_VIRTUAL_ADDRESS,
//                   program,
//                   paging_align_address((char *)program + program_size),
//                   PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
//                       PAGING_DIRECTORY_ENTRY_SUPERVISOR);
//     void *user_stack = kzalloc(USER_STACK_SIZE);
//     paging_map_to(new_task->page_directory,
//                   (void *)USER_STACK_BOTTOM,
//                   user_stack,
//                   paging_align_address((char *)user_stack + USER_STACK_SIZE),
//                   PAGING_DIRECTORY_ENTRY_IS_WRITABLE | PAGING_DIRECTORY_ENTRY_IS_PRESENT |
//                       PAGING_DIRECTORY_ENTRY_SUPERVISOR);
//     tasks_unblock(new_task);
// }

int thread_init(struct task *thread, struct process *process)
{
    thread->process = process;
    thread->process->page_directory =
        paging_create_directory(PAGING_DIRECTORY_ENTRY_IS_PRESENT | PAGING_DIRECTORY_ENTRY_SUPERVISOR);
    thread->page_directory = thread->process->page_directory;

    if (!thread->process->page_directory) {
        panic("Failed to create page directory");
        return -ENOMEM;
    }

    switch (process->file_type) {
    case PROCESS_FILE_TYPE_BINARY:
        thread->trap_frame->eip = PROGRAM_VIRTUAL_ADDRESS;
        break;
    case PROCESS_FILE_TYPE_ELF:
        thread->trap_frame->eip = elf_header(process->elf_file)->e_entry;
        break;
    default:
        panic("Unknown process file type");
        break;
    }

    thread->trap_frame->ss  = USER_DATA_SELECTOR;
    thread->trap_frame->cs  = USER_CODE_SELECTOR;
    thread->trap_frame->esp = USER_STACK_TOP;

    return ALL_OK;
}

struct task *thread_create(struct process *process)
{
    int res = 0;

    struct task *thread = tasks_new(nullptr, nullptr, TASK_PAUSED, process->file_name, USER_MODE);

    if (!thread) {
        panic("Failed to allocate memory for thread\n");
        res = -ENOMEM;
        goto out;
    }

    res = thread_init(thread, process);
    if (res != ALL_OK) {
        panic("Failed to initialize thread\n");
        goto out;
    }

out:
    if (ISERR(res)) {
        // task_free(thread);
        return ERROR(res);
    }

    return thread;
}

void *thread_virtual_to_physical_address(const struct task *thread, void *virtual_address)
{
    return paging_get_physical_address(thread->process->page_directory, virtual_address);
}


void current_task_page()
{
    if (current_task) {
        paging_switch_directory(current_task->page_directory);
    }
}