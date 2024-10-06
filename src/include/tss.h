#ifndef TSS_H
#define TSS_H

// #include <stdint.h>

// // https://wiki.osdev.org/Task_State_Segment
// struct tss
// {
//     uint32_t link;
//     uint32_t esp0; // stack pointer to use during interrupt
//     uint32_t ss0;  // stack segment to use during interrupt
//     uint32_t esp1;
//     uint32_t esp2;
//     uint32_t ss2;
//     uint32_t sr3;
//     uint32_t eip;
//     uint32_t eflags;
//     uint32_t eax;
//     uint32_t ecx;
//     uint32_t edx;
//     uint32_t ebx;
//     uint32_t esp;
//     uint32_t ebp;
//     uint32_t esi;
//     uint32_t edi;
//     uint32_t es;
//     uint32_t cs;
//     uint32_t ss;
//     uint32_t ds;
//     uint32_t fs;
//     uint32_t gs;
//     uint32_t ldtr;
//     uint32_t iopb;
// } __attribute__((packed));

// void tss_load(int tss_selector);


/* http://jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html */

#include <stdint.h>

extern struct tss_entry tss;

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp_0;
    uint16_t ss_0;
    uint16_t pad0;
    uint32_t esp_1;
    uint16_t ss_1;
    uint16_t pad1;
    uint32_t esp_2;
    uint16_t ss_2;
    uint16_t pad2;
    uint32_t reserved;
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
    uint16_t es;
    uint16_t pad3;
    uint16_t cs;
    uint16_t pad4;
    uint16_t ss;
    uint16_t pad5;
    uint16_t ds;
    uint16_t pad6;
    uint16_t fs;
    uint16_t pad7;
    uint16_t gs;
    uint16_t pad8;
    uint16_t ldt_selector;
    uint16_t pad9;
    uint16_t debug_trap;
    uint16_t iomap_base;
} __attribute ((packed));

void init_tss(void);


#endif