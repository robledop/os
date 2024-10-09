#include "my_fat.h"
#include "ata.h"
#include "terminal.h"

#include <stdint.h>
#include "string.h"
#include "memory.h"
#include "serial.h"

#define SECTOR_SIZE 512

// Function to read a sector from disk (to be implemented according to your OS)
extern int disk_read(uint32_t lba, uint8_t *buffer);

// BIOS Parameter Block structure
typedef struct
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
} __attribute__((packed)) BPB_t;

// Directory Entry structure
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

// Global variables to hold filesystem info
BPB_t bpb;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t total_clusters;

// Function to initialize the FAT16 driver
int my_fat16_init()
{
    uint8_t sector[SECTOR_SIZE];

    // Read the first sector (boot sector)
    if (disk_read(0, sector) != 0)
    {
        kprintf("Failed to read boot sector\n");
        return -1; // Error reading disk
    }

    kprintf("Read boot sector\n");

    // Copy BPB data
    memcpy(&bpb, &sector[0], sizeof(BPB_t));

    // Calculate values needed for navigation
    root_dir_sectors = ((bpb.root_entries * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
    first_data_sector = bpb.reserved_sectors + (bpb.fat_copies * bpb.sectors_per_fat) + root_dir_sectors;
    total_clusters = (bpb.total_sectors - first_data_sector) / bpb.sectors_per_cluster;

    kprintf("Bytes per sector: %s\n", bpb.oem_name);
    kprintf("Sectors per cluster: %d\n", bpb.sectors_per_cluster);
    kprintf("Reserved sectors: %d\n", bpb.reserved_sectors);
    kprintf("Number of FATs: %d\n", bpb.fat_copies);
    kprintf("Root entry count: %d\n", bpb.root_entries);
    return 0; // Success
}

// Function to read a cluster
int fat16_read_cluster(uint16_t cluster, uint8_t *buffer)
{
    uint32_t first_sector = first_data_sector + ((cluster - 2) * bpb.sectors_per_cluster);
    for (uint8_t i = 0; i < bpb.sectors_per_cluster; i++)
    {
        if (disk_read(first_sector + i, buffer + (i * bpb.bytes_per_sector)) != 0)
        {
            kprintf("Failed to read sector %d\n", first_sector + i);
            return -1; // Error reading disk
        }
    }
    kprintf("Read cluster %d\n", cluster);
    return 0; // Success
}

// Function to read the root directory
int fat16_read_root_dir(DirectoryEntry_t *entries, uint16_t max_entries)
{
    uint32_t root_dir_sector = bpb.reserved_sectors + (bpb.fat_copies * bpb.sectors_per_fat);
    uint16_t entries_read = 0;
    uint8_t sector[SECTOR_SIZE];

    for (uint32_t i = 0; i < root_dir_sectors; i++)
    {
        if (disk_read(root_dir_sector + i, sector) != 0)
        {
            kprintf("Failed to read root directory sector %d\n", root_dir_sector + i);
            return -1; // Error reading disk
        }

        for (uint16_t j = 0; j < bpb.bytes_per_sector / sizeof(DirectoryEntry_t); j++)
        {
            DirectoryEntry_t *entry = (DirectoryEntry_t *)&sector[j * sizeof(DirectoryEntry_t)];
            if (entry->name[0] == 0x00)
            {
                // No more entries
                return entries_read;
            }
            if (entry->name[0] != 0xE5 && (entry->attr & 0x0F) != 0x0F)
            {
                // Valid entry
                if (entries_read < max_entries)
                {
                    entries[entries_read++] = *entry;
                }
                else
                {
                    return entries_read; // Max entries read
                }
            }
        }
    }
    return entries_read;
}

// Function to read a file by its directory entry
int fat16_read_file(DirectoryEntry_t *file_entry, uint8_t *buffer)
{
    uint16_t cluster = file_entry->first_cluster;
    uint32_t bytes_read = 0;
    uint32_t file_size = file_entry->file_size;

    while (cluster < 0xFFF8)
    {
        if (fat16_read_cluster(cluster, buffer + bytes_read) != 0)
        {
            kprintf("Failed to read cluster %d\n", cluster);
            return -1; // Error reading cluster
        }
        bytes_read += bpb.sectors_per_cluster * bpb.bytes_per_sector;
        if (bytes_read >= file_size)
        {
            break; // File read complete
        }
        // Read next cluster from FAT
        uint32_t fat_offset = bpb.reserved_sectors * bpb.bytes_per_sector + cluster * 2;
        uint32_t fat_sector = fat_offset / bpb.bytes_per_sector;
        uint32_t ent_offset = fat_offset % bpb.bytes_per_sector;

        uint8_t sector[SECTOR_SIZE];
        if (disk_read(fat_sector, sector) != 0)
        {
            kprintf("Failed to read FAT sector %d\n", fat_sector);
            return -1; // Error reading FAT
        }
        cluster = *(uint16_t *)&sector[ent_offset];
    }
    return 0; // Success
}

void test()
{
    // struct disk *disk = disk_get(0);
    // char buffer[disk->sector_size];
    // int res = disk_read_block(disk, 3174, 1, buffer);
    // kprintf("Read block: %s, res: %d\n", buffer, res);

    kprintf("Initializing FAT16 driver\n");
    // Initialize the FAT16 driver
    if (my_fat16_init() != 0)
    {
        kprintf("Failed to initialize FAT16 driver\n");
    }

    // Read the root directory
    DirectoryEntry_t entries[256];
    int num_entries = fat16_read_root_dir(entries, 256);

    kprintf("Number of entries: %d\n", num_entries);
    for (int i = 0; i < num_entries; i++)
    {
        // kprintf("Entry size: %d\n", entries[i].file_size);
        kprintf("Entry: %s\n", (char *)entries[i].name);
        if (strncmp((char *)entries[i].name, "HELLO", 5) == 0)
        {
            uint8_t file_buffer[65536]; // Adjust size as needed
            if (fat16_read_file(&entries[i], file_buffer) != 0)
            {
                kprintf("Failed to read file\n");
            }
            kprintf("File content: %s\n", file_buffer);
        }
    }
}