#include <debug.h>

/// @brief Prints the call stack, as a list of function addresses
/// gdb or addr2line can be used to translate these into function names
void debug_callstack(void)
{
#pragma GCC diagnostic ignored "-Wframe-address"

    kprintf("Call stack: %p", __builtin_return_address(0));
    for (void **frame = __builtin_frame_address(1); (uintptr_t)frame >= 0x1000 && frame[0] != NULL; frame = frame[0])
        kprintf(" %p", frame[1]);
    kprintf(".\n");

#pragma GCC diagnostic pop
}
