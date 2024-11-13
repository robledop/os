#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <stdint.h>

// Tests with the FAT16 file system

#define FAT_DIRENT_NEVER_USED 0x00
#define FAT_DIRENT_REALLY_0E5 0x05
#define FAT_DIRENT_DIRECTORY_ALIAS 0x2e
#define FAT_DIRENT_DELETED 0xe5

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

// End of cluster chain marker
#define FAT16_EOC 0xFFF8

// Bad cluster marker
#define FAT16_BAD_CLUSTER 0xFFF7

typedef struct {
    uint8_t jmp_boot[3];         // Jump instruction to boot code
    uint8_t oem_name[8];         // OEM Name
    uint16_t bytes_per_sector;   // Bytes per sector (e.g., 512)
    uint8_t sectors_per_cluster; // Sectors per cluster
    uint16_t reserved_sectors;   // Reserved sectors (usually 1)
    uint8_t number_of_FATs;      // Number of FATs (usually 2)
    uint16_t root_entry_count;   // Number of root directory entries
    uint16_t total_sectors_16;   // Total sectors (if FAT16)
    uint8_t media_type;          // Media descriptor
    uint16_t FAT_size_16;        // Sectors per FAT
    uint16_t sectors_per_track;  // Sectors per track (for CHS)
    uint16_t number_of_heads;    // Number of heads (for CHS)
    uint32_t hidden_sectors;     // Hidden sectors
    uint32_t total_sectors_32;   // Total sectors (if FAT32)
} __attribute__((packed)) FAT16_BPB;

typedef struct {
    char name[8];       // File name
    char ext[3];        // File name
    uint8_t attributes; // File attributes
    uint8_t reserved;   // Reserved byte
    // Millisecond stamp at file creation time. This field actually
    // contains a count of tenths of a second. The granularity of the
    // seconds part of DIR_CrtTime is 2 seconds so this field is a
    // count of tenths of a second and its valid value range is 0-199
    // inclusive.
    uint8_t creation_time_tenths; // Fine creation time
    uint16_t creation_time;       // Creation time
    uint16_t creation_date;       // Creation date
    uint16_t last_access_date;    // Last access date
    uint16_t first_cluster_high;  // High word of first cluster (FAT32 only)
    uint16_t write_time;          // Last write time
    uint16_t write_date;          // Last write date
    uint16_t first_cluster;       // First cluster of the file
    uint32_t file_size;           // File size in bytes
} __attribute__((packed)) FAT16_DirEntry;

int my_fat16_init(void);
FAT16_DirEntry *fat_find_item(char *path);
void fat_read_file(FAT16_DirEntry dirEntry, uint8_t *output);
