#ifndef MY_FAT_H
#define MY_FAT_H
#include "types.h"

// Tests with the FAT16 file system

typedef struct
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t ignore_in_fat12_fat16;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster;
    uint32_t file_size;
} __attribute__((packed)) DirectoryEntry_t;

void my_fat_init();

DirectoryEntry_t *my_fat16_get_file(const char *filename);
int my_fat16_read_file(DirectoryEntry_t *file_entry, uint8_t *buffer);

#endif