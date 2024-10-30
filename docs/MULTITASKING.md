# Multi-threading

The state of the current user thread is saved by the interrupt handler when an interrupt occurs.

<pre>
    void interrupt_handler(const int interrupt, const struct interrupt_frame *frame)
    {
        uint32_t error_code;
        asm volatile("movl %%eax, %0\n" : "=r"(error_code) : : "ebx", "eax");

        kernel_page();
        if (interrupt_callbacks[interrupt] != nullptr) {
            <mark>scheduler_save_current_thread(frame);</mark>
            interrupt_callbacks[interrupt](interrupt, error_code);
        }

        scheduler_switch_current_thread_page();
        outb(0x20, 0x20);
    }
</pre>  

When the scheduler (called by an interrupt) decides to switch to another thread, the current thread's state was
already saved by the interrupt handler. So, it just needs to restore the state of the next thread and jump to it
by calling the `scheduler_switch_thread` function, which will switch to the thread's page directory, load their
previously saved registers and jump to it.
