#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <elf.h>
#include "kernel.h"
#include "scheduler.h"
#include "vga_buffer.h"

#define DEBUG_ASSERT

struct symbol {
    elf32_addr address;
    char *name;
};

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

#define FUNCTION_SYMBOL 0x02

void debug_callstack(void);
void init_symbols(const multiboot_info_t *mbd);
struct symbol debug_symbol_lookup(elf32_addr address);
