#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <assert.h>
#include <elf.h>
#include <kernel.h>
#include <scheduler.h>
#include <stdarg.h>
#include <vga_buffer.h>

#define DEBUG_ASSERT

struct symbol {
    elf32_addr address;
    char *name;
};


#define FUNCTION_SYMBOL 0x02

void debug_stats(void);
void stack_trace(void);
void init_symbols(const multiboot_info_t *mbd);
struct symbol debug_function_symbol_lookup(elf32_addr address);
