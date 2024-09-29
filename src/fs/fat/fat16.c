#include "fat16.h"
#include "status.h"
#include "string/string.h"
#include "memory/memory.h"
#include "terminal/terminal.h"
#include <stdint.h>
#include "disk/disk.h"
#include "disk/stream.h"
#include "memory/heap/kheap.h"
#include "kernel/kernel.h"
#include "config.h"

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

struct fat_directory
{
    struct fat_directory_entry *entries;
    int entry_count;
    int sector_position;
    int ending_sector_position;
};

struct fat_item
{
    union
    {
        struct fat_directory_entry *item;
        struct fat_directory *directory;
    };
    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor
{
    struct fat_item *item;
    uint32_t position;
};

struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    struct disk_stream *cluster_read_stream;
    struct disk_stream *fat_read_stream;
    struct disk_stream *directory_stream;
};

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode);

struct file_system fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
};

struct file_system *fat16_init()
{
    strncpy(fat16_fs.name, "FAT16", 20);
    // print_line(fat16_fs.name);
    return &fat16_fs;
}

static void fat16_init_private(struct disk *disk, struct fat_private *fat_private)
{
    memset(&fat_private->header, 0, sizeof(struct fat_h));

    fat_private->cluster_read_stream = disk_stream_create(disk->id);
    fat_private->fat_read_stream = disk_stream_create(disk->id);
    fat_private->directory_stream = disk_stream_create(disk->id);
}

int fat16_sector_to_absolute(struct disk *disk, int sector)
{
    return sector * disk->sector_size;
}

int fat16_get_total_items_for_directory(struct disk *disk, uint32_t directory_start_sector)
{
    struct fat_directory_entry entry;
    struct fat_directory_entry empty_entry;

    memset(&empty_entry, 0, sizeof(empty_entry));
    struct fat_private *fat_private = disk->fs_private;
    int res = 0;
    int i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream *stream = fat_private->directory_stream;
    if (disk_stream_seek(stream, directory_start_pos) != ALL_OK)
    {
        print_line("Failed to seek to directory start");
        res = -EIO;
        goto out;
    }

    while (1)
    {
        if (disk_stream_read(stream, &entry, sizeof(entry)) != ALL_OK)
        {
            print_line("Failed to read directory entry");
            res = -EIO;
            goto out;
        }

        if (entry.name[0] == 0x00)
        {
            // done
            break;
        }

        if (entry.name[0] == 0xE5)
        {
            // empty entry
            continue;
        }

        // print("Reading entry: ");
        // char *name = trim(substring((char *)entry.name, 0, 7));
        // print(name);
        // print(".");
        // sprint_line((char *)entry.ext, 3);
        // print("Entry attributes: ");
        // print_line(hex_to_string(entry.attributes));
        // print("Entry size: ");
        // print_line(int_to_string(sizeof(entry)));

        // kfree(name);

        i++;
    }

    res = i;

out:
    return res;
}

int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory)
{
    int res = 0;
    struct fat_header *primary_header = &fat_private->header.primary_header;
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = fat_private->header.primary_header.root_entries;
    int root_dir_size = root_dir_entries * sizeof(struct fat_directory_entry);
    int total_sectors = root_dir_size / disk->sector_size;
    if (root_dir_size % disk->sector_size != 0)
    {
        total_sectors++;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    struct fat_directory_entry *dir = kzalloc(total_items * sizeof(struct fat_directory_entry));
    if (!dir)
    {
        print_line("Failed to allocate memory for root directory");
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream *stream = fat_private->directory_stream;
    if (disk_stream_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != ALL_OK)
    {
        print_line("Failed to seek to root directory");
        res = -EIO;
        goto out;
    }

    if (disk_stream_read(stream, dir, root_dir_size) != ALL_OK)
    {
        print_line("Failed to read root directory");
        res = -EIO;
        goto out;
    }

    directory->entries = dir;
    directory->entry_count = total_items;
    directory->sector_position = root_dir_sector_pos;
    directory->ending_sector_position = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}

int fat16_resolve(struct disk *disk)
{
    // print("Disk type: ");
    // print_line(int_to_string(disk->type));
    // print("Disk sector size: ");
    // print_line(int_to_string(disk->sector_size));
    // print("Disk fs: ");
    // print_line(int_to_string((int)disk->fs));
    // print("Disk fs name: ");
    // print_line(disk->fs->name);

    int res = 0;

    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->fs = &fat16_fs;

    struct disk_stream *stream = disk_stream_create(disk->id);
    if (!stream)
    {
        print_line("Failed to create disk stream");
        res = -ENOMEM;
        goto out;
    }

    if (disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header)) != ALL_OK)
    {
        print_line("Failed to read FAT16 header");
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        print("Invalid FAT16 signature: ");
        print_line(int_to_string(fat_private->header.shared.extended_header.signature));
        print("File system not supported\n");

        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_OK)
    {
        print_line("Failed to get root directory");
        res = -EIO;
        goto out;
    }

    // print("OEM Name: ");
    // char *oem_name = trim(substring((char *)fat_private->header.primary_header.oem_name, 0, 7));
    // sprint_line(oem_name, 8);

    // print("Volume label: ");
    // char *volume_label = trim(substring((char *)fat_private->header.shared.extended_header.volume_label, 0, 10));
    // sprint_line(volume_label, 11);

    // print("System ID: ");
    // char* system_id = trim(substring((char *)fat_private->header.shared.extended_header.system_id_string, 0, 10));
    // sprint_line(system_id, 11);
    // print("Root directory entries: ");
    // print_line(int_to_string(fat_private->root_directory.entry_count));

    // kfree(oem_name);
    // kfree(system_id);
    // kfree(volume_label);

out:
    if (stream)
    {
        disk_stream_close(stream);
    }
    if (res < 0)
    {
        kfree(fat_private);
        disk->fs_private = NULL;
    }

    return res;
}

void fat16_to_proper_string(char **out, const char *in, size_t size)
{
    int i = 0;
    while (*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in += 1;
        // We cant process anymore since we have exceeded the input buffer size
        if (i >= size - 1)
        {
            break;
        }
        i++;
    }

    **out = 0x00;
}

void fat16_get_full_relative_filename(struct fat_directory_entry *entry, char *out, int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char *)entry->name, sizeof(entry->name));
    if (entry->ext[0] != 0x00 && entry->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char *)entry->ext, sizeof(entry->ext));
    }
}

struct fat_directory_entry *fat16_clone_fat_directory_entry(struct fat_directory_entry *entry, int size)
{
    struct fat_directory_entry *new_entry = 0;
    if (size < sizeof(struct fat_directory_entry))
    {
        print_line("Invalid size for cloning directory entry");
        return 0;
    }

    new_entry = kzalloc(size);
    if (!new_entry)
    {
        print_line("Failed to allocate memory for new entry");
        return 0;
    }

    memcpy(new_entry, entry, size);

    return new_entry;
}

static uint32_t fat16_get_first_cluster(struct fat_directory_entry *entry)
{
    // return (entry->cluster_high << 16) | entry->cluster_low;
    return entry->cluster_high | entry->cluster_low;
}

static int fat16_cluster_to_sector(struct fat_private *fat_private, int cluster)
{
    return fat_private->root_directory.ending_sector_position + ((cluster - 2) * fat_private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_sector(struct fat_private *fat_private)
{
    return fat_private->header.primary_header.reserved_sectors;
}

static int fat16_get_fat_entry(struct disk *disk, int cluster)
{
    int res = -1;
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream = fs_private->fat_read_stream;
    if (!stream)
    {
        print_line("Invalid FAT stream");
        res = -EIO;
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_sector(fs_private) * disk->sector_size;
    res = disk_stream_seek(stream, fat_table_position * (cluster * FAT16_FAT_ENTRY_SIZE));
    if (res < 0)
    {
        print_line("Failed to seek to FAT table position");
        goto out;
    }

    uint16_t result = 0;
    res = disk_stream_read(stream, &result, sizeof(result));
    if (res < 0)
    {
        print_line("Failed to read FAT table");
        goto out;
    }

    res = result;

out:
    return res;
}

static int fat16_get_cluster_for_offset(struct disk *disk, int start_cluster, int offset)
{
    int res = 0;
    struct fat_private *fs_private = disk->fs_private;
    int size_of_cluster = fs_private->header.primary_header.sectors_per_cluster * disk->sector_size;

    int cluster_to_use = start_cluster;
    int clusters_ahead = offset / size_of_cluster;

    for (int i = 0; i < clusters_ahead; i++)
    {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);

        // - 0xFF8 to 0xFFF: These values are reserved to mark the end of a cluster chain.
        // When you encounter any value in this range in the FAT (File Allocation Table), it
        // signifies that the current cluster is the last cluster of the file.
        // This means there are no more clusters to read for this file.
        if (entry == 0xFF8 || entry == 0xFFF)
        {
            // end of file
            res = -EIO;
            goto out;
        }

        if (entry == FAT16_FAT_BAD_SECTOR)
        {
            print_line("Bad sector found in FAT table");
            res = -EIO;
            goto out;
        }

        if (entry == 0xFF0 || entry == 0xFF6)
        {
            print_line("Reserved sector found in FAT table");
            res = -EIO;
            goto out;
        }

        if (entry == FAT16_UNUSED)
        {
            print_line("Corrupted sector found in FAT table");
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

out:
    return res;
}

static int fat16_read_internal_from_stream(struct disk *disk, struct disk_stream *stream, int start_cluster, int offset, int total, void *out)
{
    int res = 0;
    struct fat_private *fs_private = disk->fs_private;
    int size_of_cluster = fs_private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, start_cluster, offset);
    if (cluster_to_use < 0)
    {
        print_line("Failed to get cluster for offset");
        res = cluster_to_use;
        goto out;
    }

    int offset_in_cluster = offset % size_of_cluster;
    int start_sector = fat16_cluster_to_sector(fs_private, cluster_to_use);
    int start_position = (start_sector * disk->sector_size) + offset_in_cluster;
    // int start_position = (start_sector * disk->sector_size) * offset_in_cluster;
    int total_bytes_read = total > size_of_cluster ? size_of_cluster : total;

    res = disk_stream_seek(stream, start_position);
    if (res != ALL_OK)
    {
        print_line("Failed to seek to start position");
        goto out;
    }

    res = disk_stream_read(stream, out, total_bytes_read);
    if (res != ALL_OK)
    {
        print_line("Failed to read from stream");
        goto out;
    }

    total -= total_bytes_read;
    if (total > 0)
    {
        res = fat16_read_internal_from_stream(disk, stream, cluster_to_use, offset + total_bytes_read, total, out + total_bytes_read);
    }

out:
    return res;
}

static int fat16_read_internal(struct disk *disk, int start_cluster, int offset, int total, void *out)
{
    struct fat_private *fs_private = disk->fs_private;
    struct disk_stream *stream = fs_private->cluster_read_stream;

    return fat16_read_internal_from_stream(disk, stream, start_cluster, offset, total, out);
}

void fat16_free_directory(struct fat_directory *directory)
{
    if (!directory)
    {
        return;
    }

    if (directory->entries)
    {
        kfree(directory->entries);
    }
    kfree(directory);
}

void fat16_fat_item_free(struct fat_item *item)
{
    if (!item)
    {
        return;
    }

    if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if (item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}

struct fat_directory *fat16_load_fat_directory(struct disk *disk, struct fat_directory_entry *entry)
{
    int res = 0;
    struct fat_directory *directory = 0;
    struct fat_private *fat_private = disk->fs_private;
    if (!(entry->attributes & FAT_FILE_SUBDIRECTORY))
    {
        print_line("Invalid directory entry");
        res = -EINVARG;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory)
    {
        print_line("Failed to allocate memory for directory");
        res = -ENOMEM;
        goto out;
    }

    int cluster = fat16_get_first_cluster(entry);
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->entry_count = total_items;
    int directory_size = directory->entry_count * sizeof(struct fat_directory_entry);
    directory->entries = kzalloc(directory_size);
    if (!directory->entries)
    {
        print_line("Failed to allocate memory for directory entries");
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->entries);
    if (res != ALL_OK)
    {
        print_line("Failed to read directory entries");
        goto out;
    }

out:
    if (res != ALL_OK)
    {
        fat16_free_directory(directory);
    }
    return directory;
}

struct fat_item *fat16_new_fat_item_for_directory_entry(struct disk *disk, struct fat_directory_entry *entry)
{
    struct fat_item *f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item)
    {
        print_line("Failed to allocate memory for fat item");
        return NULL;
    }

    if (entry->attributes & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, entry);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_fat_directory_entry(entry, sizeof(struct fat_directory_entry));
    return f_item;
}

struct fat_item *fat16_find_item_in_directory(struct disk *disk, struct fat_directory *directory, const char *name)
{
    struct fat_item *f_item = 0;
    char tmp_filename[MAX_PATH_LENGTH];
    for (int i = 0; i < directory->entry_count; i++)
    {
        fat16_get_full_relative_filename(&directory->entries[i], tmp_filename, sizeof(tmp_filename));
        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
        {
            f_item = fat16_new_fat_item_for_directory_entry(disk, &directory->entries[i]);
        }
    }

    return f_item;
}

struct fat_item *fat16_get_directory_entry(struct disk *disk, struct path_part *path)
{
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item = 0;
    struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);

    if (!root_item)
    {
        print("Failed to find item: ");
        print_line(path->part);
        goto out;
    }

    struct path_part *next_part = path->next;
    current_item = root_item;
    while (next_part != 0)
    {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }

        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return current_item;
}

void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode)
{
    if (mode != FILE_MODE_READ)
    {
        print_line("Only read mode is supported for FAT16");
        return ERROR(-ERDONLY);
    }

    struct fat_file_descriptor *descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
    {
        print_line("Failed to allocate memory for file descriptor");
        return ERROR(-ENOMEM);
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item)
    {
        print_line("Failed to get directory entry");
        kfree(descriptor);
        return ERROR(-EIO);
    }

    descriptor->position = 0;

    return descriptor;
}
