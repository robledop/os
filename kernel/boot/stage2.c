#include <config.h>

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

uint8_t inb(uint16_t p);
uint16_t inw(uint16_t p);
void outb(uint16_t portid, uint8_t value);
int ata_read_sector(int lba, int total, void *buffer);

void load_kernel() {
    // This will read from sector 2 up to 512
    // They need to be marked as reserved in the boot sector (boot.asm)
    // and the kernel needs to be small enough to fit there
    // You can calculate how many sectors the kernel will use
    // by running stat -c %s ./bin/kernel.bin
    // and then dividing by 512

    // Sector 0 is the first stage bootloader
    // Sector 1 is the second stage bootloader
    // So we start reading from sector 2

    // reads 256 sectors starting from sector 2 to 257
    ata_read_sector(2, 256, (void *)KERNEL_LOAD_ADDRESS);

    // Split the command in two because the ATA PIO mode only allows 256 sectors to be read at a time
    // I may need to do more reads if the kernel grows

    // reads 254 sectors starting from sector 258 to 512
    ata_read_sector(258, 254, (void *)(KERNEL_LOAD_ADDRESS + 512 * 256));

    // Starts running kernel.asm
    auto const kernel_asm = (void (*)())KERNEL_LOAD_ADDRESS;
    kernel_asm();
}

int ata_read_sector(const int lba, const int total, void *buffer) {
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (lba & 0xFF));
    outb(0x1F4, (lba >> 8));
    outb(0x1F5, (lba >> 16));
    outb(0x1F7, 0x20);

    auto ptr = (unsigned short *)buffer;
    for (int b = 0; b < total; b++) {
        uint8_t status = inb(0x1F7);

        while ((status & 0x08) == 0) {
            status = inb(0x1F7);
        }

        // Read data
        for (int i = 0; i < 256; i++) {
            *ptr = inw(0x1F0);
            ptr++;
        }
    }

    return 0;
}

uint8_t inb(uint16_t p) {
    uint8_t r;
    asm volatile("inb %%dx, %%al" : "=a"(r) : "d"(p));
    return r;
}

uint16_t inw(uint16_t p) {
    uint16_t r;
    asm volatile("inw %%dx, %%ax" : "=a"(r) : "d"(p));
    return r;
}

void outb(uint16_t portid, uint8_t value) { asm volatile("outb %%al, %%dx" ::"d"(portid), "a"(value)); }