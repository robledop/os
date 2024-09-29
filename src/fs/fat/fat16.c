#include "fat16.h"
#include "status.h"
#include "string/string.h"
#include "terminal/terminal.h"
#include <stdint.h>

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_FAT_BAD_SECTOR 0xFF7
#define FAT16_UNUSED 0x00

// For internal use
typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// FAT Directory entry attributes
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVE 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80

struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header
{
    uint8_t jmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_copies;
    uint16_t root_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large;
} __attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_entry
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode);

struct file_system fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
};

struct file_system *fat16_init()
{
    strncpy(fat16_fs.name, "FAT16", 20);
    print_line(fat16_fs.name);
    return &fat16_fs;
}

int fat16_resolve(struct disk *disk)
{
    print_line("Resolving FAT16");
    return 0;
}

void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode)
{
    print_line("Opening FAT16");
    return 0;
}
