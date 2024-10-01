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

// int disk_read_sector(int lba, int total, void *buffer) {
//     dbgprintf("Reading sector %d from ATAPI device\n", lba);

//     // Send the IDENTIFY PACKET DEVICE command
//     outb(0x1F6, 0xA0); // Select the drive
//     outb(0x1F7, 0xA1); // Send IDENTIFY PACKET DEVICE command

//     // Wait for the drive to be ready
//     while ((inb(0x1F7) & 0x80) != 0);

//     // Check if the drive is ATAPI
//     unsigned char cl = inb(0x1F4);
//     unsigned char ch = inb(0x1F5);
//     if (cl == 0x14 && ch == 0xEB) {
//         dbgprintf("ATAPI device found\n");
//     } else {
//         dbgprintf("No ATAPI device found\n");
//         return -1;
//     }

//     // Select the drive
//     outb(0x1F6, 0x00);

//     // Send the packet command
//     outb(0x1F1, 0x00);
//     outb(0x1F2, 0x00);
//     outb(0x1F3, 0x00);
//     outb(0x1F4, 0x00);
//     outb(0x1F5, 0x00);
//     outb(0x1F7, 0xA0);

//     // Wait for the drive to be ready
//     while ((inb(0x1F7) & 0x80) != 0);

//     // Prepare the packet command
//     unsigned char packet[12] = {0xA8, 0, (lba >> 24) & 0xFF, (lba >> 16) & 0xFF, (lba >> 8) & 0xFF, lba & 0xFF, 0, 0, 0, 1, 0, 0};

//     // Send the packet command
//     for (int i = 0; i < 6; i++)
//     {
//         outw(0x1F0, ((unsigned short *)packet)[i]);
//     }

//     // Wait for the drive to be ready to send data
//     while ((inb(0x1F7) & 0x08) == 0){
//         dbgprintf("Waiting for drive to be ready\n");
//     };

//     unsigned short *ptr = (unsigned short *)buffer;
//     for (int b = 0; b < total; b++)
//     {
//         // Read data
//         for (int i = 0; i < 256; i++)
//         {
//             *ptr = inw(0x1F0);
//             ptr++;
//         }
//     }

//     dbgprintf("Read sector %d from ATAPI device\n", lba);
//     return 0;
// }

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
    dbgprintf("Reading block from disk %d, lba: %d, total: %d\n", idisk->id, lba, total);
    if (idisk != &disk)
    {
        dbgprintf("Invalid disk\n");
        return -EIO;
    }

    return disk_read_sector(lba, total, buffer);
}