#pragma once

#include "kernel.h"
#include "scheduler.h"
#include "vga_buffer.h"

#define DEBUG_ASSERT

#ifdef DEBUG_ASSERT
#define ASSERT(condition, message)                                                                                     \
    if (!(condition)) {                                                                                                \
        kprintf(KRED "Assertion failed in" KYEL " %s" KRED " on line" KYEL " %d." KRED " Process:" KYEL " %s" KWHT,    \
                __FILE__,                                                                                              \
                __LINE__,                                                                                              \
                scheduler_get_current_task()->process->file_name);                                                     \
        panic(message);                                                                                                \
    }
#else
#define ASSERT(condition, message)
#endif
