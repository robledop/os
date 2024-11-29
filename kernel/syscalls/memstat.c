#include <kernel_heap.h>
#include <syscall.h>

// TODO: Re-implement this in userland using a device file
void *sys_memstat(struct interrupt_frame *frame)
{
    kernel_heap_print_stats();

    return nullptr;
}
