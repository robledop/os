#include "ata.h"
#include "io.h"
#include "memory.h"
#include "config.h"
#include "status.h"
#include <terminal.h>
#include "serial.h"

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
        // Wait for the drive to be ready to send data
        dbgprintf("Waiting for drive to be ready\n");
        char status = inb(0x1F7);

        dbgprintf("Status: %x\n", status);

        while ((status & 0x08) == 0)
        {
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

    dbgprintf("Read sector %d\n", lba);
    return 0;
}

void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_PHYSICAL;
    disk.sector_size = SECTOR_SIZE;
    disk.id = 0;
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
        dbgprintf("Invalid disk\n");
        return -EIO;
    }

    return disk_read_sector(lba, total, buffer);
}