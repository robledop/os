#include <stdint.h>
#include "kernel.h"


__attribute__((noreturn)) void __stack_chk_fail(void)
{
    panic("Stack smashing detected");

    __builtin_unreachable();
}