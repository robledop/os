#include "my_fat.h"
#include "ata.h"
#include "terminal.h"

#include "string.h"
#include "memory.h"
#include "kernel_heap.h"
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

// Global variables to hold filesystem info
BPB_t bpb;
uint32_t root_dir_sectors;
uint32_t first_data_sector;
uint32_t total_clusters;

void my_fat16_get_null_terminated_string(char **out, const char *in, size_t size)
{
    size_t i = 0;
    while (*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in += 1;

        if (i >= size - 1)
        {
            break;
        }
        i++;
    }

    **out = 0x00;
}

void my_fat16_get_relative_filename(DirectoryEntry_t *entry, char *out, int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    my_fat16_get_null_terminated_string(&out_tmp, (const char *)entry->name, sizeof(entry->name));
    if (entry->ext[0] != 0x00 && entry->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        my_fat16_get_null_terminated_string(&out_tmp, (const char *)entry->ext, sizeof(entry->ext));
    }
}

// Function to initialize the FAT16 driver
int my_fat16_init()
{
    uint8_t sector[SECTOR_SIZE];

    // Read the first sector (boot sector)
    if (disk_read(0, sector) != 0)
    {
        dbgprintf("Failed to read boot sector\n");
        return -1; // Error reading disk
    }

    dbgprintf("Read boot sector\n");

    // Copy BPB data
    memcpy(&bpb, &sector[0], sizeof(BPB_t));

    uint32_t total_sectors = bpb.total_sectors != 0 ? bpb.total_sectors : bpb.total_sectors_large;

    // Calculate values needed for navigation
    root_dir_sectors = ((bpb.root_entries * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector;
    first_data_sector = bpb.reserved_sectors + (bpb.fat_copies * bpb.sectors_per_fat) + root_dir_sectors;
    total_clusters = (total_sectors - first_data_sector) / bpb.sectors_per_cluster;

    dbgprintf("Root directory sectors: %d\n", root_dir_sectors);
    dbgprintf("First data sector: %d\n", first_data_sector);
    dbgprintf("Total clusters: %d\n", total_clusters);

    dbgprintf("OEM name: %s\n", bpb.oem_name);
    dbgprintf("Sectors per cluster: %d\n", bpb.sectors_per_cluster);
    dbgprintf("Reserved sectors: %d\n", bpb.reserved_sectors);
    dbgprintf("Number of FATs: %d\n", bpb.fat_copies);
    dbgprintf("Root entry count: %d\n", bpb.root_entries);
    return 0; // Success
}

// Function to read a cluster
int my_fat16_read_cluster(uint16_t cluster, uint8_t *buffer)
{
    uint32_t first_sector = first_data_sector + ((cluster - 2) * bpb.sectors_per_cluster);
    for (uint8_t i = 0; i < bpb.sectors_per_cluster; i++)
    {
        if (disk_read(first_sector + i, buffer + (i * bpb.bytes_per_sector)) != 0)
        {
            dbgprintf("Failed to read sector %d\n", first_sector + i);
            return -1; // Error reading disk
        }
    }

    return 0; // Success
}

// Function to read the root directory
int my_fat16_read_root_dir(DirectoryEntry_t *entries, uint16_t max_entries)
{
    uint32_t root_dir_sector = bpb.reserved_sectors + (bpb.fat_copies * bpb.sectors_per_fat);
    uint16_t entries_read = 0;
    uint8_t sector[SECTOR_SIZE];

    for (uint32_t i = 0; i < root_dir_sectors; i++)
    {
        if (disk_read(root_dir_sector + i, sector) != 0)
        {
            dbgprintf("Failed to read root directory sector %d\n", root_dir_sector + i);
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

int my_fat16_read_file(DirectoryEntry_t *file_entry, uint8_t *buffer)
{
    uint16_t cluster = file_entry->first_cluster;
    uint32_t bytes_read = 0;
    uint32_t file_size = file_entry->file_size;

    while (cluster < 0xFFF8)
    {
        if (my_fat16_read_cluster(cluster, buffer + bytes_read) != 0)
        {
            dbgprintf("Failed to read cluster %d\n", cluster);
            return -1;
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
            dbgprintf("Failed to read FAT sector %d\n", fat_sector);
            return -1;
        }
        cluster = *(uint16_t *)&sector[ent_offset];
    }
    return 0; // Success
}

DirectoryEntry_t *my_fat16_get_file(const char *filename)
{
    if (istrncmp(filename, "0:/", 3) == 0)
    {
        filename += 3;
    }

    DirectoryEntry_t entries[10];
    int num_entries = my_fat16_read_root_dir(entries, 10);

    for (int i = 0; i < num_entries; i++)
    {
        char name[108];
        my_fat16_get_relative_filename(&entries[i], name, sizeof(name));
        if (istrncmp(name, filename, 8) == 0)
        {
            DirectoryEntry_t *entry = kzalloc(sizeof(DirectoryEntry_t));
            memcpy(entry, &entries[i], sizeof(DirectoryEntry_t));

            return entry;
        }
    }

    return NULL;
}

void test()
{
    dbgprintf("Initializing FAT16 driver\n");
    if (my_fat16_init() != 0)
    {
        dbgprintf("Failed to initialize FAT16 driver\n");
    }
}