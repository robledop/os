#include "gdt.h"
#include "tss.h"
#include "kernel.h"
#include "serial.h"

// void encode_gdt_entry(uint8_t *target, struct gdt_structured source)
// {
//     if (source.limit > 65536 && (source.limit & 0xFFF) != 0xFFF)
//     {
//         panic("encode_gdt_entry: invalid argument\n");
//     }

//     if (source.limit > 65536)
//     {
//         source.limit = source.limit >> 12;
//         target[6] = 0xC0;
//     }
//     else
//     {
//         target[6] = 0x40;
//     }

//     // limit
//     target[0] = source.limit & 0xFF;
//     target[1] = (source.limit >> 8) & 0xFF;
//     target[6] |= (source.limit >> 16) & 0x0F;

//     // base
//     target[2] = source.base & 0xFF;
//     target[3] = (source.base >> 8) & 0xFF;
//     target[4] = (source.base >> 16) & 0xFF;
//     target[7] = (source.base >> 24) & 0xFF;

//     // type
//     target[5] = source.type;
// }

// void gdt_structured_to_gdt(struct gdt *gdt, struct gdt_structured *gdt_structured, int size)
// {
//     for (int i = 0; i < size; i++)
//     {
//         encode_gdt_entry((uint8_t *)&gdt[i], gdt_structured[i]);
//     }
// }

#define LOAD_GDT() asm volatile("lgdt %0" : : "m"(gdt_addr))

static struct gdt_segment gdt[7];

struct gdt_address
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_address gdt_addr;

void gdt_set_segment(struct gdt_segment *segment, uint32_t base, uint32_t limit, char type, char privilege, char system)
{
    segment->limit_lo = (uint16_t)limit;
    segment->limit_hi = (uint8_t)(limit >> 16);
    segment->base_lo = (uint16_t)base;
    segment->base_mid = (uint8_t)(base >> 16);
    segment->base_hi = (uint8_t)(base >> 24);
    segment->access = type | system << 4 | privilege << 5 | 1 << 7;
    segment->limit_hi |= 1 << 7 | 1 << 6;
}

void init_gdt()
{
    gdt_addr.limit = 7 * 8 - 1;
    gdt_addr.base = (uint32_t)gdt;

    gdt_set_segment(gdt + GDT_KERNEL_CODE, 0, 0xfffff, GDT_CODE_SEGMENT, KERNEL_PRIVILEGE, MEMORY);
    gdt_set_segment(gdt + GDT_KERNEL_DATA, 0, 0xfffff, GDT_DATA_SEGMENT, KERNEL_PRIVILEGE, MEMORY);
    gdt_set_segment(gdt + GDT_USER_CODE, 0, 0xfffff, GDT_CODE_SEGMENT, PROCESS_PRIVILEGE, MEMORY);
    gdt_set_segment(gdt + GDT_USER_DATA, 0, 0xfffff, GDT_DATA_SEGMENT, PROCESS_PRIVILEGE, MEMORY);
    gdt_set_segment(gdt + GDT_TSS_INDEX, (uint32_t)&tss, sizeof(tss) - 1, GDT_TSS_SEGMENT, KERNEL_PRIVILEGE, SYSTEM);

    LOAD_GDT();

    /* Reload the Segment registers*/
    asm volatile("pushl %ds");
    asm volatile("popl %ds");

    asm volatile("pushl %es");
    asm volatile("popl %es");

    asm volatile("pushl %ss");
    asm volatile("popl %ss");
}
