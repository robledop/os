#ifndef TSS_H
#define TSS_H

#include "types.h"

struct tss_entry
{
    uint32_t prev_tss; // The previous TSS (if hardware task switching)
    uint32_t esp0;     // The stack pointer to load when changing to kernel mode
    uint32_t ss0;      // The stack segment to load when changing to kernel mode
    uint32_t esp1;     // Unused
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;  // Value to load into ES after task switch
    uint32_t cs;  // Value to load into CS after task switch
    uint32_t ss;  // Value to load into SS after task switch
    uint32_t ds;  // Value to load into DS after task switch
    uint32_t fs;  // Value to load into FS after task switch
    uint32_t gs;  // Value to load into GS after task switch
    uint32_t ldt; // Unused
    uint16_t trap;
    uint16_t iomap_base;

} __attribute__((packed));

void write_tss(int num, uint16_t ss0, uint32_t esp0);
void set_kernel_stack(uint32_t stack);

#endif