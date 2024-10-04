#include "gdt.h"
#include "kernel.h"
#include "serial.h"

void encode_gdt_entry(uint8_t *target, struct gdt_structured source)
{
    if (source.limit > 65536 && (source.limit & 0xFFF) != 0xFFF)
    {
        panic("encode_gdt_entry: invalid argument\n");
    }

    if (source.limit > 65536)
    {
        source.limit = source.limit >> 12;
        target[6] = 0xC0;
    }
    else
    {
        target[6] = 0x40;
    }

    // limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] |= (source.limit >> 16) & 0x0F;

    // base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    // type
    target[5] = source.type;
}

void gdt_structured_to_gdt(struct gdt *gdt, struct gdt_structured *gdt_structured, int size)
{
    for (int i = 0; i < size; i++)
    {
        encode_gdt_entry((uint8_t *)&gdt[i], gdt_structured[i]);
    }
}