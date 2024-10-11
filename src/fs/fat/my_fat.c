#include "my_fat.h"
#include "ata.h"
#include "terminal.h"
#include "kernel.h"
#include "string.h"
#include "memory.h"
#include "kernel_heap.h"
#include "serial.h"
#include "disk.h"

extern struct disk disk;
FAT16_BPB bpb;
FAT16_DirEntry *fat_get_root_entries();

FAT16_BPB fat_init()
{
    uint8_t buffer[disk.sector_size];

    disk_read(0, buffer); // Boot sector is always at LBA 0
    memcpy(&bpb, buffer, sizeof(FAT16_BPB));

    return bpb;
}

uint16_t fat_get_entry(uint16_t cluster)
{
    uint32_t fatOffset = cluster * 2; // FAT16 uses 2 bytes per entry
    uint32_t fatSector = bpb.reserved_sectors + (fatOffset / bpb.bytes_per_sector);
    uint32_t entOffset = fatOffset % bpb.bytes_per_sector;

    uint8_t buffer[bpb.bytes_per_sector];
    disk_read(fatSector, buffer);

    return *(uint16_t *)&buffer[entOffset];
}

void fat_read_root_directory()
{
    FAT16_DirEntry *root_entries = fat_get_root_entries();
    for (int i = 0; i < bpb.root_entry_count; i++)
    {
        if ((unsigned char)root_entries[i].name[0] != 0x00 && (unsigned char)root_entries[i].name[0] != 0xE5)
        {
            kprintf("File: %s\n", root_entries[i].name);
        }
    }

    kfree(root_entries);
}

FAT16_DirEntry *fat_get_root_entries()
{
    uint32_t rootDirLBA = bpb.reserved_sectors + (bpb.number_of_FATs * bpb.FAT_size_16);
    uint16_t rootEntries = bpb.root_entry_count;
    FAT16_DirEntry *dir = kzalloc(rootEntries * sizeof(FAT16_DirEntry));

    for (uint32_t i = 0; i < (rootEntries * sizeof(FAT16_DirEntry)) / bpb.bytes_per_sector; i++)
    {
        uint8_t buffer[bpb.bytes_per_sector];
        disk_read(rootDirLBA + i, buffer);
        memcpy(&dir[i * (bpb.bytes_per_sector / sizeof(FAT16_DirEntry))], buffer, bpb.bytes_per_sector);
    }

    return dir;
}

int fat_get_file(const char *filename, FAT16_DirEntry *out)
{
    FAT16_DirEntry *root_entries = fat_get_root_entries();

    for (int i = 0; i < bpb.root_entry_count; i++)
    {
        if (istrncmp(root_entries[i].name, filename, 11) == 0)
        {
            memcpy(out, &root_entries[i], sizeof(FAT16_DirEntry));
            kfree(root_entries);
            return 0;
        }
    }

    kfree(root_entries);
    return -1;
}

void fat_read_file(FAT16_DirEntry dirEntry, uint8_t *output)
{
    uint16_t currentCluster = dirEntry.first_cluster; // Start at the first cluster of the file
    uint32_t bytesRemaining = dirEntry.file_size;     // Total file size to read
    uint8_t buffer[bpb.bytes_per_sector];             // Buffer for reading one sector at a time
    uint32_t outputOffset = 0;                        // Offset in the output buffer

    while (currentCluster < FAT16_EOC && bytesRemaining > 0)
    {
        dbgprintf("Reading cluster %d\n", currentCluster);
        // Calculate the first sector (LBA) of the current cluster
        uint32_t firstSectorOfCluster = bpb.reserved_sectors + (bpb.number_of_FATs * bpb.FAT_size_16) +
                                        ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector +
                                        (currentCluster - 2) * bpb.sectors_per_cluster;

        // Read each sector in the cluster
        for (uint8_t sectorOffset = 0; sectorOffset < bpb.sectors_per_cluster && bytesRemaining > 0; sectorOffset++)
        {
            uint32_t currentSector = firstSectorOfCluster + sectorOffset;
            disk_read(currentSector, buffer); // Read the sector into buffer

            // Determine how much data to copy from this sector (might be less than 512 bytes at the end of the file)
            uint32_t bytesToCopy = (bytesRemaining < bpb.bytes_per_sector) ? bytesRemaining : bpb.bytes_per_sector;

            // Copy the read data to the output buffer
            memcpy(output + outputOffset, buffer, bytesToCopy);

            // Update the output buffer offset and the remaining bytes count
            outputOffset += bytesToCopy;
            bytesRemaining -= bytesToCopy;
        }

        // Get the next cluster from the FAT chain
        currentCluster = fat_get_entry(currentCluster);
        if (currentCluster == FAT16_BAD_CLUSTER)
        {
            panic("Bad cluster found\n");
        }
    }
}

int fat_get_sub_directory(const char *dirname, FAT16_DirEntry *out)
{
    FAT16_DirEntry *root_entries = fat_get_root_entries();

    for (int i = 0; i < bpb.root_entry_count; i++)
    {
        if (istrncmp(root_entries[i].name, dirname, 11) == 0)
        {
            memcpy(out, &root_entries[i], sizeof(FAT16_DirEntry));
            kfree(root_entries);
            return 0;
        }
    }

    kfree(root_entries);
    return -1;
}

int my_fat16_init()
{
    fat_init();

    FAT16_DirEntry file;
    fat_get_file("HELLO   TXT", &file);

    uint8_t *buffer = kmalloc(file.file_size);

    fat_read_file(file, buffer);
    kprintf("File contents: %s\n", buffer);
    // dbgprintf("File contents: %s\n", buffer);
    kfree(buffer);

    fat_read_root_directory();

    FAT16_DirEntry boot_dir;
    fat_get_sub_directory("boot       ", &boot_dir);

    panic("This is the end");
    return 0;
}