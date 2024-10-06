#ifndef GDT_H
#define GDT_H
// #include <stdint.h>

// // https://wiki.osdev.org/Global_Descriptor_Table

// #define TOTAL_GDT_SEGMENTS 6

// struct gdt
// {
//     uint16_t segment;
//     uint16_t base_first;
//     uint8_t base;
//     uint8_t access;
//     uint8_t high_flags;
//     uint8_t base_24_31_bits;
// } __attribute__((packed));

// struct gdt_structured
// {
//     uint32_t base;
//     uint32_t limit;
//     uint8_t type;
// };

// void gdt_init();
// void gdt_load(struct gdt *gdt, int size);
// void gdt_structured_to_gdt(struct gdt *gdt, struct gdt_structured *gdt_structured, int size);

#include <stdint.h>

#define KERNEL_PRIVILEGE 0
#define PROCESSS_PRIVILEGE 3
#define GDT_KERNEL_CODE 1
#define GDT_KERNEL_DATA 2
#define GDT_USER_CODE 3
#define GDT_USER_DATA 4
#define GDT_TSS_INDEX 5

// #define GDT_KERNEL_CS (GDT_KERNEL_CODE << 3)
// #define GDT_KERNEL_DS (GDT_KERNEL_DATA << 3)
// #define GDT_USER_CS (GDT_PROCESS_CODE << 3)
// #define GDT_USER_DS (GDT_PROCESS_DATA << 3)
#define GDT_KERNEL_TSS (GDT_TSS_INDEX << 3)

#define GDT_CODE_SEGMENT 0x0A
#define GDT_DATA_SEGMENT 0x02
#define GDT_TSS_SEGMENT 0x09

#define MEMORY 1
#define SYSTEM 0

void init_gdt();

struct gdt_segment
{
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_hi;
    uint8_t base_hi;
} __attribute__((packed));

#endif