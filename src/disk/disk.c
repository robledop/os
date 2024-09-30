#include "disk.h"
#include "io.h"
#include "memory.h"
#include "config.h"
#include "status.h"
#include <terminal.h>

struct disk disk;

// https://wiki.osdev.org/ATA_read/write_sectors
int disk_read_sector(int lba, int total, void *buffer)
{
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
        char status = inb(0x1F7);
        while ((status & 0x08) == 0)
        {
            status = inb(0x1F7);
        }

        // Read data
        for (int i = 0; i < 256; i++)
        {
            *ptr = inw(0x1F0);
            ptr++;
        }
    }

    return 0;
}

void disk_search_and_init()
{
    // print("Initializing disk\n");
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
    if (idisk != &disk)
    {
        return -EIO;
    }

    return disk_read_sector(lba, total, buffer);
}