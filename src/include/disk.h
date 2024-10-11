#ifndef DISK_H
#define DISK_H

#include "types.h"

typedef unsigned int DISK_TYPE;

#define DISK_TYPE_PHYSICAL 0

struct disk
{
    int id;
    DISK_TYPE type;
    int sector_size;

    struct file_system *fs;

    void *fs_private;
};

void disk_init();
struct disk *disk_get(int index);
int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buffer);
int disk_read(uint32_t lba, uint8_t *buffer);

#endif