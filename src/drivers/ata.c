#include "ata.h"
#include "io.h"
#include "memory.h"
#include "config.h"
#include "status.h"
#include <terminal.h>
#include "serial.h"
#include "kernel.h"

struct disk disk;

// https://wiki.osdev.org/ATA_read/write_sectors
int disk_read_sector(int lba, int total, void *buffer)
{
    dbgprintf("Reading sector %d\n", lba);

    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)(lba & 0xFF));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    unsigned short *ptr = (unsigned short *)buffer;
    for (int b = 0; b < total; b++)
    {
        dbgprintf("Waiting for drive to be ready\n");
        char status = inb(0x1F7);

        dbgprintf("Status: %x\n", status);

        while ((status & 0x08) == 0)
        {
            if (status & 0x01)
            {
                warningf("Error: Drive fault\n");
                panic("Error: Drive fault\n");
                return -EIO;
            }

            dbgprintf("Status: %x\n", status);
            status = inb(0x1F7);
        }

        // Read data
        for (int i = 0; i < 256; i++)
        {
            *ptr = inw(0x1F0);
            ptr++;
        }
    }

    // Check if anything was read
    dbgprintf("Read: %x\n", *(unsigned short *)buffer);

    return 0;
}

void disk_search_and_init()
{
    dbgprintf("Searching for disks\n");
    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_PHYSICAL;
    disk.id = 0;

    // Identify the drive and get the sector size
    outb(0x1F6, 0xA0); // Select drive
    outb(0x1F7, 0xEC); // Send IDENTIFY command

    // Wait for the drive to signal that it is ready
    while ((inb(0x1F7) & 0x08) == 0)
    {
    }

    unsigned short identify_data[256];
    for (int i = 0; i < 256; i++)
    {
        identify_data[i] = inw(0x1F0);
    }

    // Extract bits from word 106
    unsigned short word_106 = identify_data[106];
    int multiple_logical_sectors = (word_106 & (1 << 13)) >> 13;
    int exponent_N = (word_106 >> 4) & 0xFF;

    if (multiple_logical_sectors)
    {
        int logical_sectors_per_physical = 1 << exponent_N;
        disk.sector_size = logical_sectors_per_physical * 512; // Assuming 512-byte logical sectors
    }
    else
    {
        disk.sector_size = 512; // Logical and physical sector sizes are the same
    }

    // Validate the sector size
    if (disk.sector_size != 512 && disk.sector_size != 1024 && disk.sector_size != 2048 && disk.sector_size != 4096)
    {
        warningf("Invalid sector size detected: %d\n", disk.sector_size);
        panic("Invalid sector size detected\n");
        return;
    }

    dbgprintf("Detected sector size: %d\n", disk.sector_size);
    kprintf("Detected sector size: %d\n", disk.sector_size);

    disk.fs = fs_resolve(&disk);
}

struct disk *disk_get(int index)
{
    if (index != 0)
    {
        return 0;
    }

    return &disk;
}

int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buffer)
{
    dbgprintf("Reading block from disk %d, lba: %d, total: %d\n", idisk->id, lba, total);
    if (idisk != &disk)
    {
        warningf("Invalid disk\n");
        return -EIO;
    }

    return disk_read_sector(lba, total, buffer);
}

int disk_read(uint32_t lba, uint8_t *buffer)
{
    dbgprintf("Reading from disk %d\n", lba);
    return disk_read_block(&disk, lba, 1, buffer);
}