#include <ata.h>
#include <debug.h>
#include <disk.h>
#include <kernel.h>
#include <memory.h>

__attribute__((nonnull)) struct file_system *vfs_resolve(struct disk *disk);
struct disk disk;

void disk_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_PHYSICAL;
    disk.id   = 0;

    disk.sector_size = ata_get_sector_size();

    // Validate the sector size
    if (disk.sector_size != 512 && disk.sector_size != 1024 && disk.sector_size != 2048 && disk.sector_size != 4096) {
        panic("Invalid sector size detected\n");
        return;
    }

    disk.fs = vfs_resolve(&disk);
}

struct disk *disk_get(const int index)
{
    if (index != 0) {
        return nullptr;
    }

    return &disk;
}

int disk_read_block(const uint32_t lba, const int total, void *buffer)
{
    return ata_read_sector(lba, total, buffer);
}

int disk_read_sector(const uint32_t sector, uint8_t *buffer)
{
    return disk_read_block(sector, 1, buffer);
}

int disk_write_block(const uint32_t lba, const int total, void *buffer)
{
    return ata_write_sector(lba, total, buffer);
}

int disk_write_sector(const uint32_t sector, uint8_t *buffer)
{
    return disk_write_block(sector, 1, buffer);
}

int disk_write_sector_offset(const void *data, const int size, const int offset, const int sector)
{
    ASSERT(size <= 512 - offset);

    uint8_t buffer[512];
    disk_read_sector(sector, buffer);

    memcpy(&buffer[offset], data, size);
    return disk_write_sector(sector, buffer);
}
