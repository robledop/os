#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif
#include <stdint.h>

typedef unsigned int DISK_TYPE;

#define DISK_TYPE_PHYSICAL 0

struct disk {
    int id;
    DISK_TYPE type;
    uint16_t sector_size;
    struct file_system *fs;
    void *fs_private;
};

void disk_init(void);
struct disk *disk_get(int index);
int disk_read_block(uint32_t lba, int total, void *buffer);
int disk_read_sector(uint32_t sector, uint8_t *buffer);
__attribute__((nonnull)) int disk_write_block(uint32_t lba, int total, void *buffer);
__attribute__((nonnull)) int disk_write_sector(uint32_t sector, uint8_t *buffer);
__attribute__((nonnull)) int disk_write_sector_offset(const void *data, int size, int offset, int sector);
