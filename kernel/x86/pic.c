#include <debug.h>
#include <io.h>
#include <pic.h>

// https://wiki.osdev.org/8259_PIC

#define PIC1_PORT_A 0x20
#define PIC1_PORT_B 0x21

#define PIC2_PORT_A 0xA0
#define PIC2_PORT_B 0xA1

#define PIC1_ICW1 0x11 // Initialize the PIC and enable ICW4
#define PIC2_ICW1 PIC1_ICW2

#define PIC1_ICW2 0x20 // IRQ 0-7 will be remapped to interrupts 32 - 39
#define PIC2_ICW2 0x28 // IRQ 8-15 will be remapped to interrupts 40 - 47

#define PIC1_ICW3 0x04 // PIC1 is connected to PIC2 via IRQ2
#define PIC2_ICW3 0x02 // PIC2 is connected to PIC1 via IRQ1

#define PIC1_ICW4 0x05 // 8086/88 mode is enabled and PIC1 is master
#define PIC2_ICW4 0x01 // 8086/88 mode is enabled

#define PIC_EOI 0x20

void pic_init()
{
    outb(PIC1_PORT_A, PIC1_ICW1);
    // outb(PIC2_PORT_A, PIC2_ICW1);
    outb(PIC2_PORT_A, PIC1_ICW1);

    outb(PIC1_PORT_B, PIC1_ICW2);
    outb(PIC2_PORT_B, PIC2_ICW2);

    outb(PIC1_PORT_B, PIC1_ICW3);
    outb(PIC2_PORT_B, PIC2_ICW3);

    outb(PIC1_PORT_B, PIC1_ICW4);
    outb(PIC2_PORT_B, PIC2_ICW4);

    // Mask all interrupts
    // outb(PIC1_PORT_B, 0xEC);
    // outb(PIC2_PORT_B, 0xFF);
    // outb(PIC1_PORT_B, 0xFB);
    // outb(PIC2_PORT_B, 0xF7);
}

void pic_acknowledge(const int irq)
{
    ASSERT(irq >= 0x20 && irq < 0x30);

    // Acknowledge master PIC.
    outb(PIC1_PORT_A, PIC_EOI);

    // Acknowledge slave PIC if this is a slave interrupt.
    if (irq >= 0x28) {
        outb(PIC2_PORT_A, PIC_EOI);
    }
}
