#include "kernel.h"
#include "types.h"

// https://wiki.osdev.org/Stack_Smashing_Protector
// __stack_chk_guard is initialized in kernel_main

__attribute__((noreturn)) void __stack_chk_fail(void) // NOLINT(*-reserved-identifier)
{
    panic("Stack smashing detected");

    __builtin_unreachable();
}