#include "ata.h"

#include <debug.h>
#include <spinlock.h>
#include "io.h"
#include "kernel.h"
#include "serial.h"
#include "status.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_REG_DEVSEL 0x1F6                    // Device/Head register
#define ATA_REG_STATUS 0x1F7                    // Status register
#define ATA_REG_CMD 0x1F7                       // Status register
#define ATA_REG_SEC_COUNT 0x1F2                 // Sector count register
#define ATA_REG_LBA0 0x1F3                      // LBA low register
#define ATA_REG_LBA1 0x1F4                      // LBA mid register
#define ATA_REG_LBA2 0x1F5                      // LBA high register
#define ATA_REG_FEATURES 0x1F1                  // Features register
#define ATA_REG_CONTROL (ATA_PRIMARY_IO + 0x0C) // Control register
#define ATA_REG_DATA ATA_PRIMARY_IO             // Data register


#define ATA_CMD_IDENTIFY 0xEC    // Identify drive
#define ATA_CMD_CACHE_FLUSH 0xE7 // Flush cache
#define ATA_CMD_READ_PIO 0x20    // Read PIO
#define ATA_CMD_WRITE_PIO 0x30   // Write PIO

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_FAULT 0x20

#define ATA_MASTER 0xE0
#define ATA_SLAVE 0xF0

spinlock_t disk_lock = 0;

int ata_get_sector_size()
{
    return 512;
}

int ata_wait_for_ready()
{
    inb(ATA_REG_STATUS);
    inb(ATA_REG_STATUS);
    inb(ATA_REG_STATUS);
    inb(ATA_REG_STATUS);

    uint8_t status = inb(ATA_REG_STATUS);

    while ((status & ATA_STATUS_BUSY) && !(status & ATA_STATUS_DRQ)) {
        if (status & ATA_STATUS_ERR || status & ATA_STATUS_FAULT) {
            panic("Error: Drive fault\n");
            spin_unlock(&disk_lock);
            return -EIO;
        }

        status = inb(ATA_REG_STATUS);
    }
    return ALL_OK;
}

int ata_read_sectors(const uint32_t lba, const int total, void *buffer)
{
    spin_lock(&disk_lock);
    outb(ATA_REG_CONTROL, 0x02); // Disable interrupts

    outb(ATA_REG_DEVSEL, (lba >> 24 & 0x0F) | ATA_MASTER);
    outb(ATA_REG_SEC_COUNT, total); // Number of sectors to read
    outb(ATA_REG_LBA0, lba & 0xFF);
    outb(ATA_REG_LBA1, lba >> 8);
    outb(ATA_REG_LBA2, lba >> 16);
    outb(ATA_REG_CMD, ATA_CMD_READ_PIO);

    auto ptr = (uint16_t *)buffer;
    for (int b = 0; b < total; b++) {
        const int result = ata_wait_for_ready();
        if (result != ALL_OK) {
            return result;
        }
        // Read sector
        for (int i = 0; i < 256; i++) {
            ptr[i] = inw(ATA_PRIMARY_IO);
        }
        ptr += 256; // Advance the buffer 256 words (512 bytes)
    }

    spin_unlock(&disk_lock);

    return ALL_OK;
}

int ata_write_sectors(const uint32_t lba, const int total, void *buffer)
{
    spin_lock(&disk_lock);

    outb(ATA_REG_CONTROL, 0x02); // Disable interrupts

    int result = ata_wait_for_ready();
    if (result != ALL_OK) {
        return result;
    }

    outb(ATA_REG_DEVSEL, (lba >> 24 & 0x0F) | ATA_MASTER);
    ata_wait_for_ready();
    outb(ATA_REG_FEATURES, 0);
    outb(ATA_REG_SEC_COUNT, total); // Number of sectors to write
    outb(ATA_REG_LBA0, lba & 0xFF);
    outb(ATA_REG_LBA1, lba >> 8);
    outb(ATA_REG_LBA2, lba >> 16);
    outb(ATA_REG_CMD, ATA_CMD_WRITE_PIO);

    result = ata_wait_for_ready();
    if (result != ALL_OK) {
        return result;
    }

    auto ptr = (char *)buffer;
    for (int b = 0; b < total; b++) {
        // Write the sector
        for (int i = 0; i < 512; i += 2) {
            // Handle potential unaligned access
            uint16_t word = (uint8_t)ptr[i];
            if (i + 1 < 512) {
                word |= (uint16_t)(ptr[i + 1]) << 8;
            }
            outw(ATA_PRIMARY_IO, word);
            // Tiny delay between writes
            asm volatile("nop; nop; nop;");
        }
        outb(ATA_REG_CMD, ATA_CMD_CACHE_FLUSH);
        ata_wait_for_ready();
        ptr += 512; // Advance the buffer 512 bytes
    }

    spin_unlock(&disk_lock);

    return 0;
}
