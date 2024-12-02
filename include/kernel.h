#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <multiboot.h>
#include <stdint.h>

// struct cpu_context {
//     uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp;
//     uint32_t eip, eflags;
// };

struct cpu_context {
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t eip;
    uint32_t eflags;
};

void kernel_main(const multiboot_info_t *mbd, uint32_t magic);
void panic(const char *msg);
void set_kernel_mode_segments(void);
void start_shell(int console);
void system_reboot(void);
void system_shutdown(void);

void save_cpu_context(struct cpu_context *context);
void restore_cpu_context(struct cpu_context *context);

#define ERROR(x) ((void *)(x))
#define ERROR_I(x) ((int)(x))
#define ISERR(x) ((int)(x) < 0)
