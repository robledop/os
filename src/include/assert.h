#pragma once

#include "kernel.h"
#include "vga_buffer.h"

#define DEBUG_ASSERT

#ifdef DEBUG_ASSERT
#define ASSERT(condition, message)                                                                                     \
    if (!(condition)) {                                                                                                \
        kprintf(KRED "Assertion failed in %s on line %d\n" KWHT, __FILE__, __LINE__);                                  \
        panic(message);                                                                                                \
    }
#else
#define ASSERT(condition, message)
#endif
