#include "fat16.h"
#include "status.h"
#include "string.h"
#include "memory.h"
#include <stdint.h>
#include "ata.h"
#include "stream.h"
#include "kheap.h"
#include "kernel.h"
#include "config.h"
#include "serial.h"
#include "terminal.h"

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
#define FAT_FILE_LONG_NAME 0x0F

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
int fat16_read(struct disk *disk, void *private_data, uint32_t size, uint32_t nmemb, char *out);
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE whence);
int fat16_stat(struct disk *disk, void *private, struct file_stat *stat);
int fat16_close(void *private);
struct file_directory get_fs_root_directory(struct disk *disk);
struct file_directory get_subdirectory(struct disk *disk, const char *path);
struct fat_item *fat16_find_item_in_directory(struct disk *disk, struct fat_directory *directory, const char *name);

// struct file_system fat16_fs = {
//     .resolve = fat16_resolve,
//     .open = fat16_open,
//     .read = fat16_read,
//     .seek = fat16_seek,
//     .stat = fat16_stat,
//     .close = fat16_close,
//     .get_root_directory = get_fs_root_directory,
//     .get_subdirectory = get_subdirectory,
// };

struct file_system* fat16_fs;

struct file_system *fat16_init()
{
    // strncpy(fat16_fs.name, "FAT16", 20);
    // dbgprintf("File system name %s\n", fat16_fs.name);
    // return &fat16_fs;

    struct file_system *fs = kzalloc(sizeof(struct file_system));

    strncpy(fs->name, "FAT16", 20);
    fs->resolve = fat16_resolve;
    fs->close = fat16_close;
    fs->read = fat16_read;
    fs->get_root_directory = get_fs_root_directory;
    fs->get_subdirectory = get_subdirectory;
    fs->seek = fat16_seek;
    fs->stat = fat16_stat;
    fs->open = fat16_open;

    // struct file_system fat16_fs = {
    //     .resolve = fat16_resolve,
    //     .open = fat16_open,
    //     .read = fat16_read,
    //     .seek = fat16_seek,
    //     .stat = fat16_stat,
    //     .close = fat16_close,
    //     .get_root_directory = get_fs_root_directory,
    //     .get_subdirectory = get_subdirectory,
    // };


    fat16_fs = fs;

    return fs;
}

static void fat16_init_private(struct disk *disk, struct fat_private *fat_private)
{
    memset(&fat_private->header, 0, sizeof(struct fat_h));

    fat_private->cluster_read_stream = disk_stream_create(disk->id);
    fat_private->fat_read_stream = disk_stream_create(disk->id);
    fat_private->directory_stream = disk_stream_create(disk->id);
}

struct directory_entry get_directory_entry(void *fat_directory_entries, int index)
{
    // struct fat_directory_entry *entry = (struct fat_directory_entry *)entries[index];
    struct fat_directory_entry *entries = (struct fat_directory_entry *)fat_directory_entries;
    struct fat_directory_entry *entry = entries + index;

    struct directory_entry directory_entry =
        {
            // .name = (char *)entry->name,
            // .ext = (char *)entry->ext,
            .attributes = entry->attributes,
            .size = entry->size,
            .access_date = entry->access_date,
            .creation_date = entry->creation_date,
            .creation_time = entry->creation_time,
            .creation_time_tenths = entry->creation_time_tenths,
            .modification_date = entry->modification_date,
            .modification_time = entry->modification_time,
            .is_archive = entry->attributes & FAT_FILE_ARCHIVE,
            .is_device = entry->attributes & FAT_FILE_DEVICE,
            .is_directory = entry->attributes & FAT_FILE_SUBDIRECTORY,
            .is_hidden = entry->attributes & FAT_FILE_HIDDEN,
            .is_long_name = entry->attributes == FAT_FILE_LONG_NAME,
            .is_read_only = entry->attributes & FAT_FILE_READ_ONLY,
            .is_system = entry->attributes & FAT_FILE_SYSTEM,
            .is_volume_label = entry->attributes & FAT_FILE_VOLUME_LABEL,
        };

    // TODO: Check for a memory leak here
    char *name = trim(substring((char *)entry->name, 0, 7));
    char *ext = trim(substring((char *)entry->ext, 0, 2));

    directory_entry.name = name;
    directory_entry.ext = ext;

    // kfree(name);
    // kfree(ext);
    return directory_entry;
}

struct file_directory get_fs_root_directory(struct disk *disk)
{
    struct fat_private *fat_private = disk->fs_private;
    struct file_directory root_directory =
        {
            .name = "root",
            .entry_count = fat_private->root_directory.entry_count,
            .entries = fat_private->root_directory.entries,
            .get_entry = get_directory_entry,
        };
    return root_directory;
}

struct file_directory get_subdirectory(struct disk *disk, const char *path)
{
    struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path);

    if (!current_item || current_item->type != FAT_ITEM_TYPE_DIRECTORY)
    {
        warningf("Failed to find subdirectory: %s\n", path.part);
        struct file_directory empty_directory = {0};
        return empty_directory;
    }

    // char name[20];
    // strcpy((char*)name, path);

    // char *token, *last;
    // last = token = strtok((char *)name, "/");
    // for (; (token = strtok(NULL, "/")) != NULL; last = token)
    //     ;

    struct file_directory subdirectory =
        {
            .name = "sdfkjsf",
            .entry_count = current_item->directory->entry_count,
            .entries = current_item->directory->entries,
            .get_entry = get_directory_entry,
        };

    return subdirectory;
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
        dbgprintf("Failed to seek to directory start");
        res = -EIO;
        goto out;
    }

    while (1)
    {
        if (disk_stream_read(stream, &entry, sizeof(entry)) != ALL_OK)
        {
            dbgprintf("Failed to read directory entry");
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

        char *name = trim(substring((char *)entry.name, 0, 7));
        char *ext = trim(substring((char *)entry.ext, 0, 2));
        dbgprintf("Reading entry: %s.%s\n", name, ext);
        dbgprintf("Entry attributes: %x\n", entry.attributes);
        dbgprintf("Entry size: %d\n", entry.size);

        kfree(name);
        kfree(ext);

        i++;
    }

    res = i;

out:
    return res;
}

int fat16_get_root_directory(struct disk *disk, struct fat_private *fat_private, struct fat_directory *directory)
{
    dbgprintf("Getting root directory\n");

    int res = 0;
    struct fat_directory_entry *dir = NULL;
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
    dir = kzalloc(total_items * sizeof(struct fat_directory_entry));
    if (!dir)
    {
        warningf("Failed to allocate memory for root directory\n");
        res = -ENOMEM;
        goto error_out;
    }

    struct disk_stream *stream = fat_private->directory_stream;
    if (disk_stream_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != ALL_OK)
    {
        warningf("Failed to seek to root directory\n");
        res = -EIO;
        goto error_out;
    }

    if (disk_stream_read(stream, dir, root_dir_size) != ALL_OK)
    {
        warningf("Failed to read root directory\n");
        res = -EIO;
        goto error_out;
    }

    directory->entries = dir;
    directory->entry_count = total_items;
    directory->sector_position = root_dir_sector_pos;
    directory->ending_sector_position = root_dir_sector_pos + (root_dir_size / disk->sector_size);

    dbgprintf("Root directory entries: %d\n", directory->entry_count);

    return res;

error_out:
    if (dir)
    {
        kfree(dir);
    }
    return res;
}

int fat16_resolve(struct disk *disk)
{
    // kprintf("fat_resolve\n");
    int res = 0;
    dbgprintf("Disk type: %d\n", disk->type);
    dbgprintf("Disk sector size: %d\n", disk->sector_size);

    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->fs = fat16_fs;

    struct disk_stream *stream = disk_stream_create(disk->id);
    if (!stream)
    {
        warningf("Failed to create disk stream");
        res = -ENOMEM;
        goto out;
    }

    if (disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header)) != ALL_OK)
    {
        warningf("Failed to read FAT16 header\n");
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != 0x29)
    {
        dbgprintf("Invalid FAT16 signature: %x\n", fat_private->header.shared.extended_header.signature);
        dbgprintf("File system not supported\n");

        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_OK)
    {
        warningf("Failed to get root directory\n");
        res = -EIO;
        goto out;
    }

    char *oem_name = trim(substring((char *)fat_private->header.primary_header.oem_name, 0, 7));
    dbgprintf("OEM Name: %s\n", oem_name);

    char *volume_label = trim(substring((char *)fat_private->header.shared.extended_header.volume_label, 0, 10));
    dbgprintf("Volume label: %s\n", volume_label);

    char *system_id = trim(substring((char *)fat_private->header.shared.extended_header.system_id_string, 0, 10));
    dbgprintf("System ID: %s\n", system_id);

    dbgprintf("Root directory entries: %d\n", fat_private->root_directory.entry_count);

    kfree(oem_name);
    kfree(system_id);
    kfree(volume_label);

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

    dbgprintf("FAT16 resolve result: %d\n", res);

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
        warningf("Invalid size for cloning directory entry\n");
        return 0;
    }

    new_entry = kzalloc(size);
    if (!new_entry)
    {
        warningf("Failed to allocate memory for new entry\n");
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
        warningf("Invalid FAT stream\n");
        res = -EIO;
        goto out;
    }

    uint32_t fat_table_position = fat16_get_first_sector(fs_private) * disk->sector_size;
    res = disk_stream_seek(stream, fat_table_position * (cluster * FAT16_FAT_ENTRY_SIZE));
    if (res < 0)
    {
        warningf("Failed to seek to FAT table position\n");
        goto out;
    }

    uint16_t result = 0;
    res = disk_stream_read(stream, &result, sizeof(result));
    if (res < 0)
    {
        warningf("Failed to read FAT table\n");
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
            warningf("Bad sector found in FAT table\n");
            res = -EIO;
            goto out;
        }

        if (entry == 0xFF0 || entry == 0xFF6)
        {
            warningf("Reserved sector found in FAT table\n");
            res = -EIO;
            goto out;
        }

        if (entry == FAT16_UNUSED)
        {
            warningf("Corrupted sector found in FAT table\n");
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
        warningf("Failed to get cluster for offset\n");
        res = cluster_to_use;
        goto out;
    }

    int offset_in_cluster = offset % size_of_cluster;
    int start_sector = fat16_cluster_to_sector(fs_private, cluster_to_use);
    int start_position = (start_sector * disk->sector_size) + offset_in_cluster;
    int total_bytes_read = total > size_of_cluster ? size_of_cluster : total;

    res = disk_stream_seek(stream, start_position);
    if (res != ALL_OK)
    {
        warningf("Failed to seek to start position\n");
        goto out;
    }

    res = disk_stream_read(stream, out, total_bytes_read);
    if (res != ALL_OK)
    {
        warningf("Failed to read from stream\n");
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
        warningf("Invalid directory entry\n");
        res = -EINVARG;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory)
    {
        warningf("Failed to allocate memory for directory\n");
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
        warningf("Failed to allocate memory for directory entries\n");
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->entries);
    if (res != ALL_OK)
    {
        warningf("Failed to read directory entries\n");
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
        warningf("Failed to allocate memory for fat item\n");
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
        warningf("Failed to find item: %s\n", path->part);
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
    struct fat_file_descriptor *descriptor = NULL;
    int error_code = 0;
    if (mode != FILE_MODE_READ)
    {
        warningf("Only read mode is supported for FAT16\n");
        error_code = -ERDONLY;
        goto error_out;
    }

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor)
    {
        warningf("Failed to allocate memory for file descriptor\n");
        error_code = -ENOMEM;
        goto error_out;
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if (!descriptor->item)
    {
        warningf("Failed to get directory entry\n");
        error_code = -EIO;
        goto error_out;
    }

    descriptor->position = 0;

    return descriptor;

error_out:
    if (descriptor)
    {
        kfree(descriptor);
    }

    return ERROR(error_code);
}

// typedef int (*FS_READ_FUNCTION)(struct disk *disk, void *private, uint32_t size, uint32_t nmemb, char *out);
int fat16_read(struct disk *disk, void *descriptor, uint32_t size, uint32_t nmemb, char *out)
{
    int res = 0;

    struct fat_file_descriptor *fat_desc = descriptor;
    struct fat_directory_entry *entry = fat_desc->item->item;
    int offset = fat_desc->position;

    for (uint32_t i = 0; i < nmemb; i++)
    {
        res = fat16_read_internal(disk, fat16_get_first_cluster(entry), offset, size, out);
        if (ISERR(res))
        {
            warningf("Failed to read from file\n");
            return res;
        }

        out += size;
        offset += size;
    }

    res = nmemb;

    return res;
}

int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    int res = 0;

    struct fat_file_descriptor *fat_desc = private;
    struct fat_item *desc_item = fat_desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        warningf("Invalid file descriptor\n");
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_entry *entry = desc_item->item;
    if (offset > entry->size)
    {
        warningf("Offset exceeds file size\n");
        res = -EIO;
        goto out;
    }

    switch (seek_mode)
    {
    case SEEK_SET:
        fat_desc->position = offset;
        break;
    case SEEK_CURRENT:
        fat_desc->position += offset;
        break;
    case SEEK_END:
        res = -EUNIMP;
        break;
    default:
        warningf("Invalid seek mode: %d\n", seek_mode);
        res = -EINVARG;
        break;
    }

out:
    return res;
}

int fat16_stat(struct disk *disk, void *private, struct file_stat *stat)
{
    int res = 0;

    struct fat_file_descriptor *fat_desc = (struct fat_file_descriptor *)private;
    struct fat_item *desc_item = fat_desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        warningf("Invalid file descriptor\n");
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_entry *entry = desc_item->item;
    stat->size = entry->size;
    stat->flags = 0;
    if (entry->attributes & FAT_FILE_READ_ONLY)
    {
        stat->flags |= FILE_STAT_IS_READ_ONLY;
    }

out:
    return res;
}

static void fat16_free_file_descriptor(struct fat_file_descriptor *descriptor)
{
    if (!descriptor)
    {
        return;
    }

    fat16_fat_item_free(descriptor->item);
    kfree(descriptor);
}

int fat16_close(void *private)
{
    fat16_free_file_descriptor((struct fat_file_descriptor *)private);
    return ALL_OK;
}