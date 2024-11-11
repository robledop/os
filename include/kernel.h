#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <multiboot.h>
#include <types.h>

void kernel_main(const multiboot_info_t *mbd, uint32_t magic);
void panic(const char *msg);
void set_kernel_mode_segments(void);
void start_shell(int console);
void system_reboot(void);
void system_shutdown(void);

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)

// Keep track of how many times we entered a critical section
extern int __cli_count;

void enter_critical(void);
// Only leave critical section if we are the last one
void leave_critical(void);
