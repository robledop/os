#include "disk.h"
#include "ata.h"
#include "status.h"
#include "serial.h"
#include "memory.h"
#include "file.h"
#include "kernel.h"

struct disk disk;

void disk_init()
{
    dbgprintf("Searching for disks\n");
    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_PHYSICAL;
    disk.id = 0;

    disk.sector_size = ata_get_sector_size();

    // Validate the sector size
    if (disk.sector_size != 512 && disk.sector_size != 1024 && disk.sector_size != 2048 && disk.sector_size != 4096)
    {
        warningf("Invalid sector size detected: %d\n", disk.sector_size);
        panic("Invalid sector size detected\n");
        return;
    }

    dbgprintf("Detected sector size: %d\n", disk.sector_size);

    // disk.fs = fs_resolve(&disk);
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

    return ata_read_sector(lba, total, buffer);
}

int disk_read(uint32_t sector, uint8_t *buffer)
{
    dbgprintf("Reading from disk %d\n", sector);
    return disk_read_block(&disk, sector, 1, buffer);
}