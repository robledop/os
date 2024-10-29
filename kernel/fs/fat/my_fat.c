#include "my_fat.h"
#include "ata.h"
#include "vga_buffer.h"
#include "kernel.h"
#include "string.h"
#include "memory.h"
#include "kernel_heap.h"
#include "serial.h"
#include "disk.h"
#include "assert.h"

extern struct disk disk;
FAT16_BPB bpb;
FAT16_DirEntry *fat_get_root_entries();

FAT16_BPB fat_init()
{
    uint8_t buffer[disk.sector_size];

    ata_read_sector(0, 1, buffer); // Boot sector is always at LBA 0
    memcpy(&bpb, buffer, sizeof(FAT16_BPB));

    return bpb;
}

void fat_get_null_terminated_string(char **out, const char *in, size_t size)
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

void fat_get_relative_filename(FAT16_DirEntry *entry, char *out, int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat_get_null_terminated_string(&out_tmp, (const char *)entry->name, sizeof(entry->name));
    if (entry->ext[0] != 0x00 && entry->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        fat_get_null_terminated_string(&out_tmp, (const char *)entry->ext, sizeof(entry->ext));
    }
}

uint32_t fat_get_first_sector_of_cluster(uint32_t cluster)
{
    return bpb.reserved_sectors + (bpb.number_of_FATs * bpb.FAT_size_16) +
           ((bpb.root_entry_count * 32) + (bpb.bytes_per_sector - 1)) / bpb.bytes_per_sector +
           (cluster - 2) * bpb.sectors_per_cluster;
}

uint16_t fat_get_entry(uint16_t cluster)
{
    uint32_t fat_offset = cluster * 2; // FAT16 uses 2 bytes per entry
    uint32_t fat_sector = bpb.reserved_sectors + (fat_offset / bpb.bytes_per_sector);
    uint32_t entry_offset = fat_offset % bpb.bytes_per_sector;

    uint8_t buffer[bpb.bytes_per_sector];
    ata_read_sector(fat_sector, 1, buffer);

    return *(uint16_t *)&buffer[entry_offset];
}

FAT16_DirEntry *fat_get_root_entries()
{
    uint32_t first_sector_of_cluster = bpb.reserved_sectors + bpb.number_of_FATs * bpb.FAT_size_16;

    uint32_t entries_per_sector = bpb.bytes_per_sector / sizeof(FAT16_DirEntry);
    uint32_t total_entries = bpb.root_entry_count;
    FAT16_DirEntry *entries = kzalloc(total_entries * sizeof(FAT16_DirEntry));

    for (uint32_t i = 0; i < bpb.sectors_per_cluster; i++)
    {
        uint8_t buffer[bpb.bytes_per_sector];
        ata_read_sector(first_sector_of_cluster + i, 1, buffer);

        for (uint32_t j = 0; j < entries_per_sector; j++)
        {
            // Skip entries for long file names
            FAT16_DirEntry *entry = (FAT16_DirEntry *)(buffer + j * sizeof(FAT16_DirEntry));
            if (entry->attributes & ATTR_LONG_NAME)
            {
                continue;
            }

            memcpy(&entries[i * entries_per_sector + j], buffer + j * sizeof(FAT16_DirEntry), sizeof(FAT16_DirEntry));
        }
    }

    return entries;
}

FAT16_DirEntry *fat_load_directory_entries(FAT16_DirEntry *dir)
{
    uint32_t first_sector_of_cluster = fat_get_first_sector_of_cluster(dir->first_cluster);

    uint32_t entries_per_sector = bpb.bytes_per_sector / sizeof(FAT16_DirEntry);
    uint32_t total_entries = bpb.sectors_per_cluster * entries_per_sector;
    FAT16_DirEntry *entries = kzalloc(total_entries * sizeof(FAT16_DirEntry));

    for (uint32_t i = 0; i < bpb.sectors_per_cluster; i++)
    {
        uint8_t buffer[bpb.bytes_per_sector];
        ata_read_sector(first_sector_of_cluster + i, 1, buffer);

        for (uint32_t j = 0; j < entries_per_sector; j++)
        {
            // Skip entries for long file names
            FAT16_DirEntry *entry = (FAT16_DirEntry *)(buffer + j * sizeof(FAT16_DirEntry));
            if (entry->attributes & ATTR_LONG_NAME)
            {
                continue;
            }

            memcpy(&entries[i * entries_per_sector + j], buffer + j * sizeof(FAT16_DirEntry), sizeof(FAT16_DirEntry));
        }
    }

    return entries;
}

void fat_read_file(FAT16_DirEntry dir_entry, uint8_t *output)
{
    uint16_t current_cluster = dir_entry.first_cluster; // Start at the first cluster of the file
    uint32_t bytes_remaining = dir_entry.file_size;     // Total file size to read
    // uint8_t buffer[bpb.bytes_per_sector];               // Buffer for reading one sector at a time
    uint32_t output_offset = 0; // Offset in the output buffer
    uint8_t *buffer = kmalloc(bpb.bytes_per_sector);

    while (current_cluster < FAT16_EOC && bytes_remaining > 0)
    {
        dbgprintf("Reading cluster %d\n", current_cluster);
        // Calculate the first sector (LBA) of the current cluster
        uint32_t first_sector_of_cluster = fat_get_first_sector_of_cluster(current_cluster);

        // Read each sector in the cluster
        for (uint8_t sector_offset = 0; sector_offset < bpb.sectors_per_cluster && bytes_remaining > 0; sector_offset++)
        {
            uint32_t current_sector = first_sector_of_cluster + sector_offset;
            ata_read_sector(current_sector, 1, buffer); // Read the sector into buffer

            // Determine how much data to copy from this sector (might be less than 512 bytes at the end of the file)
            uint32_t bytes_to_copy = (bytes_remaining < bpb.bytes_per_sector) ? bytes_remaining : bpb.bytes_per_sector;

            // Copy the read data to the output buffer
            memcpy(output + output_offset, buffer, bytes_to_copy);

            // Update the output buffer offset and the remaining bytes count
            output_offset += bytes_to_copy;
            ASSERT(bytes_remaining >= bytes_to_copy, "Bytes remaining is less than bytes to copy");
            bytes_remaining -= bytes_to_copy;
        }

        // Get the next cluster from the FAT chain
        current_cluster = fat_get_entry(current_cluster);
        if (current_cluster == FAT16_BAD_CLUSTER)
        {
            panic("Bad cluster found\n");
        }
    }

    kfree(buffer);
}

FAT16_DirEntry *fat_find_item(char *path)
{
    char *token = strtok(path, "/");
    FAT16_DirEntry *current_part = fat_get_root_entries();
    FAT16_DirEntry result;
    FAT16_DirEntry *next_dir = NULL;
    while (token != NULL)
    {
        next_dir = NULL;
        for (int i = 0; i < bpb.root_entry_count; i++)
        {
            result = current_part[i];
            char name[12];

            fat_get_relative_filename(&result, name, 12);
            if (istrncmp(name, token, 11) == 0)
            {
                next_dir = fat_load_directory_entries(&result);
                break;
            }
        }

        if (next_dir == NULL)
        {
            return NULL;
        }

        kfree(current_part);
        current_part = next_dir;
        token = strtok(NULL, "/");
    }
    kfree(next_dir);

    FAT16_DirEntry *result_ptr = kmalloc(sizeof(FAT16_DirEntry));
    memcpy(result_ptr, &result, sizeof(FAT16_DirEntry));

    return result_ptr;
}

int my_fat16_init()
{
    fat_init();

    // FAT16_DirEntry file;
    // fat_get_item_in_root("test    TXT", &file);

    // uint8_t *buffer = kmalloc(file.file_size);

    // fat_read_file(file, buffer);
    // kprintf("%s\n", buffer);
    // dbgprintf("%s\n", buffer);

    // FAT16_DirEntry *root_entries = fat_get_root_entries();
    // for (int i = 0; i < bpb.root_entry_count; i++)
    // {
    //     if ((unsigned char)root_entries[i].name[0] != 0x00 && (unsigned char)root_entries[i].name[0] != 0xE5)
    //     {
    //         kprintf("File: %s\n", root_entries[i].name);
    //     }
    // }

    // FAT16_DirEntry boot_dir;
    // fat_get_item_in_root("BOOT       ", &boot_dir);

    // FAT16_DirEntry *entries = fat_load_directory_entries(&boot_dir);

    // for (uint8_t i = 0; i < bpb.sectors_per_cluster * bpb.bytes_per_sector / sizeof(FAT16_DirEntry); i++)
    // {
    //     if ((unsigned char)entries[i].name[0] != 0x00 && (unsigned char)entries[i].name[0] != 0xE5)
    //     {
    //         kprintf("File: %s\n", entries[i].name);
    //     }
    // }

    // FAT16_DirEntry *grub = fat_find_item("BOOT       /GRUB       /GRUB    CFG");
    // FAT16_DirEntry *kernel = fat_find_item("BOOT       /MYOS    BIN");

    // FAT16_DirEntry* hello = fat_find_item("HELLO   TXT");
    // fat_read_file(*hello, buffer);
    // kprintf("File contents: %s\n", buffer);
    // uint8_t *buffer2 = kmalloc(grub->file_size);
    // fat_read_file(*grub, buffer2);
    // kprintf("File contents: %s\n", buffer2);

    return 0;
}