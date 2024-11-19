# Task switching

*This is a working in progress and I realize the approach is naive. I'm still learning about this topic.*

The state of the current user thread is saved by the interrupt handler when an interrupt occurs.

<pre>
    void interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
    {
        if (interrupt_callbacks[interrupt] != nullptr) {
            kernel_page();
            <mark>scheduler_save_current_thread(frame);</mark>
            interrupt_callbacks[interrupt](interrupt, frame);
            scheduler_switch_current_thread_page();
        }

        if (interrupt >= 0x20 && interrupt < 0x30) {
            pic_acknowledge(interrupt);
        }
    }
</pre>  

When the scheduler (called by the timer interrupt) decides to switch to another thread, the current thread's state was
already saved by the interrupt handler. So, it just needs to restore the state of the next thread and jump to it
by calling the `scheduler_switch_thread` function, which will switch to the thread's page directory, load their
previously saved registers and jump to it.

# Scheduler

The main function is the `schedule` function. Most of the time it gets called by the timer interrupt handler.

```c
/// @brief Gets the next thread and runs it
/// @remark Must be called with interrupts disabled
void schedule()
{
    // Move the first item in the queue to the back
    scheduler_rotate_queue();

    auto next = scheduler_get_next_thread();

    // If there is no next thread, restart the shell
    if (!next) {
        printf("\nRestarting the shell");
        start_shell(0);
        next = scheduler_get_next_thread();
    }
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

    // If the next thread is in the RUNNING state, switch to it.
    scheduler_switch_thread(next);
}
```

## Terminating processes

When a process launches a child process and waits for it, the function `process_wait_pid()` gets called.
This function will look for the child process, remove it if it's a zombie, or put the parent process in WAITING state.
That will make the scheduler call `scheduler_check_waiting_thread()` which will keep checking if the child process is a
zombie or not.

If a child process is not awaited by its parent, then the `exit()` syscall will eventually be called and remove the
child
process from it's parent's children list even if the parent did not wait for it.

```c



