#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include "multiboot.h"

void kernel_main(multiboot_info_t *mbd, unsigned int magic);
void panic(const char *msg);
void kernel_page();
void set_kernel_mode_segments();
void start_shell(int console);
void system_reboot();
void system_shutdown();

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)

// Keep track of how many times we entered a critical section
extern int __cli_count;

#define ENTER_CRITICAL()                                                                                               \
    __cli_count++;                                                                                                     \
    asm volatile("cli");

// Only leave critical section if we are the last one
#define LEAVE_CRITICAL()                                                                                               \
    __cli_count--;                                                                                                     \
    if (__cli_count == 0) {                                                                                            \
        asm volatile("sti");                                                                                           \
    }
