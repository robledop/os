#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

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
                scheduler_get_current_thread()->process->file_name);                                                   \
        panic(message);                                                                                                \
    }
#else
#define ASSERT(condition, message)
#endif

void debug_callstack(void);
