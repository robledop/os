#include <config.h>
#include <debug.h>
#include <disk.h>
#include <fat16.h>
#include <inode.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memfs.h>
#include <memory.h>
#include <path_parser.h>
#include <serial.h>
#include <spinlock.h>
#include <status.h>
#include <stream.h>
#include <string.h>
#include <time.h>

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY_SIZE 0x02
#define FAT16_FAT_BAD_SECTOR 0xFFF7
#define FAT16_UNUSED 0x00
// End of cluster chain marker
#define FAT16_EOC 0xFFF8
#define FAT16_EOC2 0xFFFF

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
#define FAT_FILE_LONG_NAME 0x0F

struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

struct fat_header {
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

struct fat_h {
    struct fat_header primary_header;
    union fat_h_e {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_entry {
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
    uint16_t first_cluster;
    uint32_t size;
} __attribute__((packed));

struct fat_directory {
    struct fat_directory_entry *entries;
    int entry_count;
    int sector_position;
    uint32_t ending_sector_position;
};

struct fat_item {
    union {
        struct fat_directory_entry *item;
        struct fat_directory *directory;
    };
    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor {
    struct fat_item *item;
    uint32_t position;
    struct disk *disk;
};

struct fat_private {
    struct fat_h header;
    struct fat_directory root_directory;
    struct disk_stream *cluster_read_stream;
    struct disk_stream *fat_read_stream;
    struct disk_stream *directory_stream;
};

#define FAT_ENTRIES_PER_SECTOR (512 / sizeof(struct fat_directory_entry))

static uint8_t *fat_table   = nullptr;
spinlock_t fat16_table_lock = 0;

int fat16_resolve(struct disk *disk);
void *fat16_open(const struct path_root *path, FILE_MODE mode);
int fat16_read(const void *descriptor, size_t size, off_t nmemb, char *out);
int fat16_write(void *descriptor, const char *data, size_t size);
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode);
int fat16_stat(void *descriptor, struct file_stat *stat);
int fat16_close(void *descriptor);
time_t fat_date_time_to_unix_time(uint16_t fat_date, uint16_t fat_time);
static int fat16_get_fat_entry(const struct disk *disk, int cluster);
int fat16_create_file(const char *name, const char *ext, void *data, int data_length);

struct inode_operations fat16_inode_ops = {
    .open  = fat16_open,
    .read  = fat16_read,
    .write = fat16_write,
    .seek  = fat16_seek,
    .stat  = fat16_stat,
    .close = fat16_close,
};

struct file_system *fat16_fs;

struct file_system *fat16_init()
{
    fat16_fs = kzalloc(sizeof(struct file_system));

    fat16_fs->type    = FS_TYPE_FAT16;
    fat16_fs->resolve = fat16_resolve;

    fat16_fs->get_root_directory = fat16_get_vfs_root_directory;

    strncpy(fat16_fs->name, "FAT16", 20);
    dbgprintf("File system name %s\n", fat16_fs->name);
    ASSERT(fat16_fs->resolve != nullptr, "Resolve function is null");

    return fat16_fs;
}

static uint16_t get_fat_start_sector(const struct fat_private *fat_private)
{
    return fat_private->header.primary_header.reserved_sectors;
}

static void fat16_init_private(const struct disk *disk, struct fat_private *fat_private)
{
    memset(&fat_private->header, 0, sizeof(struct fat_h));

    fat_private->cluster_read_stream = disk_stream_create(disk->id);
    fat_private->fat_read_stream     = disk_stream_create(disk->id);
    fat_private->directory_stream    = disk_stream_create(disk->id);
}

int fat16_sector_to_absolute(const struct disk *disk, const int sector)
{
    return sector * disk->sector_size;
}

/// @brief Load the first FAT into memory
int fat16_load_table(const struct fat_private *fat_private)
{
    ASSERT(fat_table);

    const uint16_t first_fat_start_sector = fat_private->header.primary_header.reserved_sectors;
    const uint16_t sector_size            = fat_private->header.primary_header.bytes_per_sector;
    const uint16_t fat_sectors            = fat_private->header.primary_header.sectors_per_fat;

    spin_lock(&fat16_table_lock);

    for (uint16_t i = 0; i < fat_sectors; i++) {
        if (disk_read_sector(first_fat_start_sector + i, fat_table + (i * sector_size)) < 0) {
            panic("Failed to read FAT\n");
            return -EIO;
        }
    }
    spin_unlock(&fat16_table_lock);
    return ALL_OK;
}

/// @brief Write the FAT back to disk
void fat16_flush_table(const struct fat_private *fat_private)
{
    ASSERT(fat_table);

    const uint16_t start_sector = fat_private->header.primary_header.reserved_sectors;
    const uint16_t sector_size  = fat_private->header.primary_header.bytes_per_sector;
    const uint16_t fat_sectors  = fat_private->header.primary_header.sectors_per_fat;

    spin_lock(&fat16_table_lock);

    for (uint16_t i = 0; i < fat_sectors; i++) {
        if (disk_write_sector(start_sector + i, fat_table + i * sector_size) < 0) {
            panic("Failed to write FAT table\n");
        }
    }

    spin_unlock(&fat16_table_lock);
}

/// @brief Set a FAT entry in memory. This needs to be flushed to disk to persist.
void fat16_set_fat_entry(const uint32_t cluster, const uint16_t value)
{
    if (fat_table == nullptr) {
        return;
    }

    spin_lock(&fat16_table_lock);

    const uint32_t fat_offset             = cluster * FAT16_FAT_ENTRY_SIZE;
    *(uint16_t *)(fat_table + fat_offset) = value;

    spin_unlock(&fat16_table_lock);
}

uint32_t fat16_get_free_cluster(const struct disk *disk)
{
    // First 2 (2 * FAT_ENTRY_SIZE) entries are reserved
    for (int i = 5; i < 65536; i++) {
        if (fat16_get_fat_entry(disk, i) == FAT16_UNUSED) {
            fat16_set_fat_entry(i, 0xFFFF);

            return i;
        }
    }

    return -1;
}

int fat16_get_total_items_for_directory(const struct disk *disk, const uint32_t directory_start_sector)
{
    struct fat_directory_entry entry;

    const struct fat_private *fat_private = disk->fs_private;
    int res                               = 0;
    int i                                 = 0;
    ASSERT(disk->sector_size > 0, "Invalid sector size");
    const uint32_t directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream *stream         = fat_private->directory_stream;
    if (disk_stream_seek(stream, directory_start_pos) != ALL_OK) {
        warningf("Failed to seek to directory start");
        res = -EIO;
        goto out;
    }

    while (1) {
        if (disk_stream_read(stream, &entry, sizeof(entry)) != ALL_OK) {
            warningf("Failed to read directory entry");
            res = -EIO;
            goto out;
        }

        if (entry.name[0] == 0x00) {
            break;
        }

        if (entry.name[0] == 0xE5) {
            continue;
        }

        i++;
    }

    res = i;

out:
    return res;
}

int fat16_get_root_directory(const struct disk *disk, const struct fat_private *fat_private,
                             struct fat_directory *directory)
{
    dbgprintf("Getting root directory\n");

    ASSERT(disk->sector_size > 0, "Invalid sector size");

    int res                                 = 0;
    const struct fat_header *primary_header = &fat_private->header.primary_header;
    const int root_dir_sector_pos =
        (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    const int root_dir_entries   = fat_private->header.primary_header.root_entries;
    const uint32_t root_dir_size = root_dir_entries * sizeof(struct fat_directory_entry);

    const int total_items           = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    struct fat_directory_entry *dir = kzalloc(total_items * sizeof(struct fat_directory_entry));
    if (!dir) {
        warningf("Failed to allocate memory for root directory\n");
        res = -ENOMEM;
        goto error_out;
    }

    struct disk_stream *stream = fat_private->directory_stream;
    if (disk_stream_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != ALL_OK) {
        warningf("Failed to seek to root directory\n");
        res = -EIO;
        goto error_out;
    }

    if (disk_stream_read(stream, dir, root_dir_size) != ALL_OK) {
        warningf("Failed to read root directory\n");
        res = -EIO;
        goto error_out;
    }

    directory->entries                = dir;
    directory->entry_count            = total_items;
    directory->sector_position        = root_dir_sector_pos;
    directory->ending_sector_position = root_dir_sector_pos + (root_dir_size / disk->sector_size);

    dbgprintf("Root directory entries: %d\n", directory->entry_count);

    return res;

error_out:
    if (dir) {
        kfree(dir);
    }
    return res;
}

int fat16_resolve(struct disk *disk)
{
    int res                         = 0;
    struct fat_private *fat_private = kzalloc(sizeof(struct fat_private));

    if (fat_private == NULL) {
        panic("Failed to allocate memory for FAT16 private data");
        return -1;
    }
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->fs         = fat16_fs;

    struct disk_stream *stream = disk_stream_create(disk->id);
    if (!stream) {
        warningf("Failed to create disk stream");
        res = -ENOMEM;
        goto out;
    }

    if (disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header)) != ALL_OK) {
        warningf("Failed to read FAT16 header\n");
        res = -EIO;
        goto out;
    }

    if (fat_private->header.shared.extended_header.signature != 0x29) {
        warningf("Invalid FAT16 signature: %x\n", fat_private->header.shared.extended_header.signature);
        warningf("File system not supported\n");

        res = -EFSNOTUS;
        goto out;
    }

    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_OK) {
        warningf("Failed to get root directory\n");
        res = -EIO;
        goto out;
    }

    fat_table = kzalloc(fat_private->header.primary_header.sectors_per_fat * disk->sector_size);
    if (!fat_table) {
        warningf("Failed to allocate memory for FAT table\n");
        res = -ENOMEM;
        goto out;
    }

    fat16_load_table(fat_private);
    // Writing the FAT table back to disk just to test if the write works and doesn't corrupt the table
    fat16_flush_table(fat_private);

    char data[] = "Hello, World!";

    for (int i = 0; i < 5; i++) {
        char name[8];
        sprintf(name, "file%d", i);
        fat16_create_file(name, "txt", data, (int)strlen(data));
    }

    // Load it again with the new files
    if (fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_OK) {
        warningf("Failed to get root directory\n");
        res = -EIO;
        goto out;
    }


    vfs_add_mount_point("/", disk->id, nullptr);

out:
    if (stream) {
        disk_stream_close(stream);
    }
    if (res < 0) {
        kfree(fat_private);
        disk->fs_private = nullptr;
    }

    return res;
}

void fat16_get_null_terminated_string(char **out, const char *in, size_t size)
{
    size_t i = 0;
    while (*in != 0x00 && *in != 0x20) {
        **out = *in;
        *out += 1;
        in += 1;

        if (i >= size - 1) {
            break;
        }
        i++;
    }

    **out = 0x00;
}

void fat16_get_relative_filename(const struct fat_directory_entry *entry, char *out, const int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_get_null_terminated_string(&out_tmp, (const char *)entry->name, sizeof(entry->name));
    if (entry->ext[0] != 0x00 && entry->ext[0] != 0x20) {
        *out_tmp++ = '.';
        fat16_get_null_terminated_string(&out_tmp, (const char *)entry->ext, sizeof(entry->ext));
    }
}

struct fat_directory_entry *fat16_clone_fat_directory_entry(const struct fat_directory_entry *entry, const size_t size)
{
    if (size < sizeof(struct fat_directory_entry)) {
        warningf("Invalid size for cloning directory entry\n");
        return nullptr;
    }

    struct fat_directory_entry *new_entry = kzalloc(size);
    if (!new_entry) {
        warningf("Failed to allocate memory for new entry\n");
        return nullptr;
    }

    memcpy(new_entry, entry, size);

    return new_entry;
}

static uint32_t fat16_cluster_to_sector(const struct fat_private *fat_private, const int cluster)
{
    return fat_private->root_directory.ending_sector_position +
        ((cluster - 2) * fat_private->header.primary_header.sectors_per_cluster);
}

static int fat16_get_fat_entry(const struct disk *disk, const int cluster)
{
    const struct fat_private *fs_private = disk->fs_private;

    const uint32_t fat_offset = cluster * 2;
    const uint32_t fat_sector = fs_private->header.primary_header.reserved_sectors +
        (fat_offset / fs_private->header.primary_header.bytes_per_sector);
    const uint32_t fat_entry_offset = fat_offset % fs_private->header.primary_header.bytes_per_sector;
    uint16_t result                 = 0;

    uint8_t buffer[fs_private->header.primary_header.bytes_per_sector];

    int res = disk_read_sector(fat_sector, buffer);
    result  = *(uint16_t *)&buffer[fat_entry_offset];
    if (res < 0) {
        warningf("Failed to read FAT table\n");
        goto out;
    }

    res = result;

out:
    return res;
}

static int fat16_get_cluster_for_offset(const struct disk *disk, const int start_cluster, const uint32_t offset)
{
    int res                              = 0;
    const struct fat_private *fs_private = disk->fs_private;
    const int size_of_cluster            = fs_private->header.primary_header.sectors_per_cluster * disk->sector_size;

    int cluster_to_use            = start_cluster;
    const uint32_t clusters_ahead = offset / size_of_cluster;

    for (uint32_t i = 0; i < clusters_ahead; i++) {
        // DEBUG: cluster_to_use could be wrong, and it ends up making this function return 0
        const int entry = fat16_get_fat_entry(disk, cluster_to_use);

        // - 0xFFF8 to 0xFFFF: These values are reserved to mark the end of a cluster chain.
        // When you encounter any value in this range in the FAT (File Allocation Table), it
        // signifies that the current cluster is the last cluster of the file.
        // This means there are no more clusters to read for this file.
        if (entry >= 0xFFF8) {
            res = -EIO;
            goto out;
        }

        if (entry == FAT16_FAT_BAD_SECTOR) {
            res = -EIO;
            goto out;
        }

        if (entry >= 0xFFF0) {
            res = -EIO;
            goto out;
        }

        if (entry == FAT16_UNUSED) {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

out:
    return res;
}

static int fat16_read_internal(const struct disk *disk, const int cluster, const uint32_t offset, uint32_t total,
                               void *out)
{
    int res                              = 0;
    const struct fat_private *private    = disk->fs_private;
    struct disk_stream *stream           = private->cluster_read_stream;
    const uint32_t size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    const int cluster_to_use             = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0) {
        res = cluster_to_use;
        goto out;
    }

    const uint32_t offset_from_cluster = offset % size_of_cluster_bytes;

    const uint32_t starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    const uint32_t starting_pos    = (starting_sector * disk->sector_size) + offset_from_cluster;
    const uint32_t total_to_read   = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;

    res = disk_stream_seek(stream, starting_pos);
    if (res != ALL_OK) {
        goto out;
    }

    res = disk_stream_read(stream, out, total_to_read);
    if (res != ALL_OK) {
        goto out;
    }

    total -= total_to_read;
    if (total > 0) {
        // We still have more to read
        res = fat16_read_internal(disk, cluster, offset + total_to_read, total, (char *)out + total_to_read);
    }

out:
    return res;
}

void fat16_free_directory(struct fat_directory *directory)
{
    if (!directory) {
        return;
    }

    if (directory->entries) {
        kfree(directory->entries);
    }
    kfree(directory);
}

void fat16_fat_item_free(struct fat_item *item)
{
    if (!item) {
        return;
    }

    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(item->directory);
    } else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->item);
    }

    kfree(item);
}

struct fat_directory *fat16_load_fat_directory(const struct disk *disk, const struct fat_directory_entry *entry)
{
    int res                         = 0;
    struct fat_directory *directory = nullptr;
    struct fat_private *fat_private = disk->fs_private;
    if (!(entry->attributes & FAT_FILE_SUBDIRECTORY)) {
        warningf("Invalid directory entry\n");
        res = -EINVARG;
        goto out;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory) {
        warningf("Failed to allocate memory for directory\n");
        res = -ENOMEM;
        goto out;
    }

    const int cluster                 = entry->first_cluster;
    const uint32_t cluster_sector     = fat16_cluster_to_sector(fat_private, cluster);
    const int total_items             = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->entry_count            = total_items;
    const unsigned int directory_size = directory->entry_count * sizeof(struct fat_directory_entry);
    directory->entries                = kzalloc(directory_size);
    if (!directory->entries) {
        warningf("Failed to allocate memory for directory entries\n");
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->entries);
    if (res != ALL_OK) {
        warningf("Failed to read directory entries\n");
        goto out;
    }

out:
    if (res != ALL_OK) {
        fat16_free_directory(directory);
    }
    return directory;
}

struct fat_item *fat16_new_fat_item_for_directory_entry(const struct disk *disk,
                                                        const struct fat_directory_entry *entry)
{
    struct fat_item *f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item) {
        warningf("Failed to allocate memory for fat item\n");
        return nullptr;
    }

    if (entry->attributes & FAT_FILE_SUBDIRECTORY) {
        f_item->directory = fat16_load_fat_directory(disk, entry);
        f_item->type      = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_fat_directory_entry(entry, sizeof(struct fat_directory_entry));
    return f_item;
}

struct fat_item *fat16_find_item_in_directory(const struct disk *disk, const struct fat_directory *directory,
                                              const char *name)
{
    struct fat_item *f_item = nullptr;
    for (int i = 0; i < directory->entry_count; i++) {
        char tmp_filename[MAX_PATH_LENGTH];
        fat16_get_relative_filename(&directory->entries[i], tmp_filename, sizeof(tmp_filename));
        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            return fat16_new_fat_item_for_directory_entry(disk, &directory->entries[i]);
        }
    }

    return f_item;
}

struct fat_item *fat16_get_directory_entry(struct disk *disk, const struct path_part *path)
{
    dbgprintf("Getting directory entry for: %s\n", path->part);
    const struct fat_private *fat_private = disk->fs_private;
    struct fat_item *current_item         = nullptr;
    struct fat_item *root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->name);

    if (!root_item) {
        warningf("Failed to find item: %s\n", path->name);
        goto out;
    }

    const struct path_part *next_part = path->next;
    current_item                      = root_item;
    while (next_part != nullptr) {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            current_item = nullptr;
            break;
        }

        struct fat_item *tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->name);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part    = next_part->next;
    }

out:
    return current_item;
}

void *fat16_open(const struct path_root *path, const FILE_MODE mode)
{
    struct fat_file_descriptor *descriptor = nullptr;
    int error_code                         = 0;
    if (mode != FILE_MODE_READ) {
        warningf("Only read mode is supported for FAT16\n");
        error_code = -ERDONLY;
        goto error_out;
    }

    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if (!descriptor) {
        warningf("Failed to allocate memory for file descriptor\n");
        error_code = -ENOMEM;
        goto error_out;
    }

    struct disk *disk = disk_get(path->drive_number);

    descriptor->item = fat16_get_directory_entry(disk, path->first);
    if (!descriptor->item) {
        warningf("Failed to get directory entry\n");
        error_code = -EIO;
        goto error_out;
    }

    descriptor->position = 0;
    descriptor->disk     = disk;

    return descriptor;

error_out:
    if (descriptor) {
        kfree(descriptor);
    }

    return ERROR(error_code);
}


/// @brief Add a new entry to the directory
int fat16_add_entry(const uint16_t dir_sector, const char *name, const char *ext, const uint8_t attributes,
                    const uint16_t file_cluster, const uint32_t file_size)
{
    uint8_t buffer[512] = {0};

    if (disk_read_sector(dir_sector, buffer) < 0) {
        panic("Error reading block\n");
        return -1;
    }

    for (int entry = 0; entry < (int)FAT_ENTRIES_PER_SECTOR; entry++) {
        auto const dir_entry = (struct fat_directory_entry *)(buffer + entry * sizeof(struct fat_directory_entry));
        // Find an empty entry to write the new file
        if (dir_entry->name[0] == 0x00 || dir_entry->name[0] == 0xE5) {
            memset(dir_entry, 0, sizeof(struct fat_directory_entry));
            memset(dir_entry->name, ' ', 8);
            memcpy(dir_entry->name, name, 8);
            memcpy(dir_entry->ext, ext, 3);
            dir_entry->attributes    = attributes;
            dir_entry->first_cluster = file_cluster;
            dir_entry->size          = file_size;

            disk_write_sector(dir_sector, buffer);
            return ALL_OK;
        }
    }

    return -EIO;
}

void fat16_write_data_to_clusters(uint8_t *data, const uint16_t starting_cluster, const uint32_t size)
{
    const struct disk *disk               = disk_get(0);
    const struct fat_private *fat_private = disk->fs_private;
    const uint16_t bytes_per_sector       = fat_private->header.primary_header.bytes_per_sector;
    const uint8_t sectors_per_cluster     = fat_private->header.primary_header.sectors_per_cluster;
    const uint32_t bytes_per_cluster      = bytes_per_sector * sectors_per_cluster;
    const uint16_t reserved_sectors       = fat_private->header.primary_header.reserved_sectors;
    const uint8_t fat_copies              = fat_private->header.primary_header.fat_copies;
    const uint16_t sectors_per_fat        = fat_private->header.primary_header.sectors_per_fat;
    const uint16_t first_data_sector      = reserved_sectors + (fat_copies * sectors_per_fat);

    uint16_t current_cluster = starting_cluster;
    uint32_t data_offset     = 0;
    const int cluster_to_use = fat16_get_cluster_for_offset(disk, current_cluster, data_offset);

    while (current_cluster < FAT16_EOC && data_offset < size) {
        const uint32_t cluster_sector = fat16_cluster_to_sector(fat_private, cluster_to_use);
        uint32_t bytes_to_write       = bytes_per_cluster;
        if ((size - data_offset) < bytes_per_cluster) {
            bytes_to_write = size - data_offset;
        }

        disk_write_block(cluster_sector, sectors_per_cluster, data + data_offset);
        data_offset += bytes_to_write;

        // TODO: This does not work. I need to allocate a new entry in the FAT table instead
        current_cluster = fat16_get_fat_entry(disk, ++current_cluster);
    }
}


int fat16_create_file(const char *name, const char *ext, void *data, const int data_length)
{
    // HACK: This needs to be passed as an argument
    const struct disk *disk               = disk_get(0);
    const struct fat_private *fat_private = disk->fs_private;

    const uint16_t bytes_per_sector   = fat_private->header.primary_header.bytes_per_sector;
    const uint8_t sectors_per_cluster = fat_private->header.primary_header.sectors_per_cluster;
    const uint32_t bytes_per_cluster  = bytes_per_sector * sectors_per_cluster;
    const uint16_t reserved_sectors   = fat_private->header.primary_header.reserved_sectors;
    const uint8_t fat_copies          = fat_private->header.primary_header.fat_copies;
    const uint16_t sectors_per_fat    = fat_private->header.primary_header.sectors_per_fat;
    const uint16_t first_data_sector  = reserved_sectors + (fat_copies * sectors_per_fat);

    const int first_cluster = (int)fat16_get_free_cluster(disk);
    if (first_cluster < 0) {
        panic("No free cluster found\n");
        return -EIO;
    }

    struct fat_directory root_dir;
    fat16_get_root_directory(disk, fat_private, &root_dir);

    fat16_add_entry(root_dir.sector_position, (char *)name, ext, FAT_FILE_ARCHIVE, first_cluster, data_length);

    if (data_length > 0 && data != nullptr) {
        uint32_t clusters_needed = (data_length + bytes_per_cluster - 1) / bytes_per_cluster;
        fat16_write_data_to_clusters(data, first_cluster, data_length);
    }

    fat16_flush_table(fat_private);

    return 0;
}

int fat16_write(void *descriptor, const char *data, size_t size)
{
    struct file_descriptor *desc         = (struct file_descriptor *)descriptor;
    struct fat_file_descriptor *fat_desc = desc->fs_data;
    struct fat_directory_entry *entry    = fat_desc->item->item;
    struct disk *disk                    = fat_desc->disk;
    uint32_t offset                      = fat_desc->position;

    panic("Not implemented\n");

    return -EUNIMP;
}

int fat16_read(const void *descriptor, const size_t size, const off_t nmemb, char *out)
{
    int res = 0;

    auto const desc                            = (struct file_descriptor *)descriptor;
    const struct fat_file_descriptor *fat_desc = desc->fs_data;
    const struct fat_directory_entry *entry    = fat_desc->item->item;
    const struct disk *disk                    = fat_desc->disk;
    uint32_t offset                            = fat_desc->position;

    for (off_t i = 0; i < nmemb; i++) {
        res = fat16_read_internal(disk, entry->first_cluster, offset, size, out);
        if (ISERR(res)) {
            warningf("Failed to read from file\n");
            return res;
        }

        out += size;
        offset += size;
    }

    res = (int)nmemb;

    return res;
}

int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    int res = 0;

    auto const desc                      = (struct file_descriptor *)private;
    struct fat_file_descriptor *fat_desc = desc->fs_data;
    struct fat_item *desc_item           = fat_desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        warningf("Invalid file descriptor\n");
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_entry *entry = desc_item->item;
    if (offset > entry->size) {
        warningf("Offset exceeds file size\n");
        res = -EIO;
        goto out;
    }

    switch (seek_mode) {
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

int fat16_stat(void *descriptor, struct file_stat *stat)
{
    int res = 0;

    auto const desc                  = (struct file_descriptor *)descriptor;
    auto const fat_desc              = (struct fat_file_descriptor *)desc->fs_data;
    const struct fat_item *desc_item = fat_desc->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE) {
        warningf("Invalid file descriptor\n");
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_entry *entry = desc_item->item;
    stat->size                        = entry->size;
    stat->flags                       = 0;
    if (entry->attributes & FAT_FILE_READ_ONLY) {
        stat->flags |= FILE_STAT_IS_READ_ONLY;
    }

out:
    return res;
}

static void fat16_free_file_descriptor(struct fat_file_descriptor *descriptor)
{
    if (!descriptor) {
        warningf("Invalid file descriptor\n");
        return;
    }

    fat16_fat_item_free(descriptor->item);
    kfree(descriptor);
}

int fat16_close(void *descriptor)
{
    auto const desc = (struct file_descriptor *)descriptor;
    fat16_free_file_descriptor((struct fat_file_descriptor *)desc->fs_data);
    return ALL_OK;
}

struct dir_entry get_directory_entry(void *fat_directory_entries, int index)
{
    struct fat_directory_entry *entries = (struct fat_directory_entry *)fat_directory_entries;
    struct fat_directory_entry *entry   = entries + index;

    char *name_dup = kzalloc(9);
    char *ext_dup  = kzalloc(4);
    memcpy(name_dup, entry->name, 8);
    memcpy(ext_dup, entry->ext, 3);

    char *name = trim((char *)name_dup, 8);
    char *ext  = trim((char *)ext_dup, 3);

    for (size_t i = 0; i < strlen(name); i++) {
        name[i] = tolower(name[i]);
    }

    for (size_t i = 0; i < strlen(ext); i++) {
        ext[i] = tolower(ext[i]);
    }

    enum inode_type type = entry->attributes & FAT_FILE_SUBDIRECTORY ? INODE_DIRECTORY : INODE_FILE;

    auto inode        = memfs_create_inode(type, &fat16_inode_ops);
    inode->ops        = kzalloc(sizeof(struct inode_operations));
    inode->ops->open  = fat16_open;
    inode->ops->read  = fat16_read;
    inode->ops->seek  = fat16_seek;
    inode->ops->stat  = fat16_stat;
    inode->ops->close = fat16_close;

    inode->size         = entry->size;
    inode->data         = &entry;
    inode->fs_type      = FS_TYPE_FAT16;
    inode->atime        = fat_date_time_to_unix_time(entry->access_date, 0);
    inode->mtime        = fat_date_time_to_unix_time(entry->modification_date, entry->modification_time);
    inode->ctime        = fat_date_time_to_unix_time(entry->creation_date, entry->creation_time);
    inode->is_read_only = entry->attributes & FAT_FILE_READ_ONLY;
    inode->is_hidden    = entry->attributes & FAT_FILE_HIDDEN;
    inode->is_system    = entry->attributes & FAT_FILE_SYSTEM;
    inode->is_archive   = entry->attributes & FAT_FILE_ARCHIVE;

    struct dir_entry dir_entry = {
        .inode = inode,
    };

    if (strlen(ext) > 0) {
        strcat(dir_entry.name, name);
        strcat(dir_entry.name, ".");
        strcat(dir_entry.name, ext);
    } else {
        strcat(dir_entry.name, name);
    }

    dir_entry.inode->data = entry;


    return dir_entry;
}

size_t fat16_count_non_lfn_entries(const struct fat_directory directory)
{
    size_t count = 0;
    for (int i = 0; i < directory.entry_count; i++) {
        if (directory.entries[i].attributes != FAT_FILE_LONG_NAME) {
            count++;
        }
    }

    return count;
}

void fat16_convert_fat_to_vfs(const struct fat_directory *fat_directory, struct dir_entries *vfs_directory)
{
    vfs_directory->capacity = fat16_count_non_lfn_entries(*fat_directory);
    vfs_directory->count    = vfs_directory->capacity;
    vfs_directory->entries  = kzalloc(sizeof(struct dir_entry) * vfs_directory->count);

    int index = 0;
    for (int i = 0; i < fat_directory->entry_count; i++) {
        struct dir_entry entry        = get_directory_entry(fat_directory->entries, i);
        const uint8_t file_attributes = ((struct fat_directory_entry *)entry.inode->data)->attributes;

        if (file_attributes == FAT_FILE_LONG_NAME) {
            continue;
        }

        vfs_directory->entries[index] = kzalloc(sizeof(struct dir_entry));
        memcpy(vfs_directory->entries[index], &entry, sizeof(struct dir_entry));
        entry.inode->ops->open              = fat16_open;
        entry.inode->ops->read              = fat16_read;
        entry.inode->ops->seek              = fat16_seek;
        entry.inode->ops->stat              = fat16_stat;
        entry.inode->ops->close             = fat16_close;
        entry.inode->ops->get_sub_directory = fat16_get_subdirectory;
        entry.inode->ops->lookup            = memfs_lookup;
        entry.inode->fs_type                = FS_TYPE_FAT16;
        index++;
    }
}

int fat16_get_vfs_root_directory(const struct disk *disk, struct dir_entries *directory)
{
    const struct fat_private *fat_private = disk->fs_private;
    fat16_convert_fat_to_vfs(&fat_private->root_directory, directory);

    return 0;
}

int fat16_get_subdirectory(const struct path_root *root_path, struct dir_entries *directory)
{
    auto const disk = disk_get(root_path->drive_number);

    const struct fat_private *fat_private = disk->fs_private;
    auto path_part                        = root_path->first;
    struct fat_item *current_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path_part->name);
    path_part                     = path_part->next;

    while (path_part != nullptr && current_item != NULL && current_item->type == FAT_ITEM_TYPE_DIRECTORY) {
        struct fat_item *next_item = fat16_find_item_in_directory(disk, current_item->directory, path_part->name);
        fat16_fat_item_free(current_item);
        current_item = next_item;
        path_part    = path_part->next;
    }

    if (current_item == NULL) {
        return -ENOENT;
    }

    // TODO: All memory allocated here is leaked
    fat16_convert_fat_to_vfs(current_item->directory, directory);

    return 0;
}

time_t fat_date_time_to_unix_time(const uint16_t fat_date, const uint16_t fat_time)
{
    const int day   = fat_date & 0x1F;
    const int month = (fat_date >> 5) & 0x0F;
    const int year  = ((fat_date >> 9) & 0x7F) + 1980;

    const int second = (fat_time & 0x1F) * 2;
    const int minute = (fat_time >> 5) & 0x3F;
    const int hour   = (fat_time >> 11) & 0x1F;

    struct tm t = {0};
    t.tm_year   = year - 1900; // tm_year is years since 1900
    t.tm_mon    = month - 1;   // tm_mon is months since January (0-11)
    t.tm_mday   = day;
    t.tm_hour   = hour;
    t.tm_min    = minute;
    t.tm_sec    = second;
    t.tm_isdst  = -1; // Let mktime determine whether DST is in effect

    const time_t unix_time = mktime(&t);

    return unix_time;
}

#if 0

void fat16_print_partition_stats(const struct disk *disk)
{
    struct fat_private *fat_private = disk->fs_private;
    struct disk_stream *stream      = disk_stream_create(disk->id);
    if (!stream) {
        warningf("Failed to create disk stream");
        panic("Failed to create disk stream");
    }

    if (disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header)) != ALL_OK) {
        warningf("Failed to read FAT16 header\n");
        disk_stream_close(stream);
        panic("Failed to read FAT16 header");
    }

    const uint16_t sector_size         = fat_private->header.primary_header.bytes_per_sector;
    const uint8_t fat_copies           = fat_private->header.primary_header.fat_copies;
    const uint16_t heads               = fat_private->header.primary_header.heads;
    const uint16_t hidden_sectors      = fat_private->header.primary_header.hidden_sectors;
    const uint16_t media_type          = fat_private->header.primary_header.media_type;
    const uint16_t reserved_sectors    = fat_private->header.primary_header.reserved_sectors;
    const uint16_t root_entries        = fat_private->header.primary_header.root_entries;
    const uint16_t sectors_per_cluster = fat_private->header.primary_header.sectors_per_cluster;
    const uint16_t sectors_per_fat     = fat_private->header.primary_header.sectors_per_fat;
    const uint16_t sectors_per_track   = fat_private->header.primary_header.sectors_per_track;
    const uint16_t total_sectors       = fat_private->header.primary_header.total_sectors != 0
              ? fat_private->header.primary_header.total_sectors
              : fat_private->header.primary_header.total_sectors_large;

    dbgprintf("Sector size: %d\n", sector_size);
    dbgprintf("FAT copies: %d\n", fat_copies);
    dbgprintf("Heads: %d\n", heads);
    dbgprintf("Hidden sectors: %d\n", hidden_sectors);
    dbgprintf("Media type: %d\n", media_type);
    dbgprintf("OEM name: %s\n", fat_private->header.primary_header.oem_name);
    dbgprintf("Reserved sectors: %d\n", reserved_sectors);
    dbgprintf("Root entries: %d\n", root_entries);
    dbgprintf("Sectors per cluster: %d\n", sectors_per_cluster);
    dbgprintf("Sectors per FAT: %d\n", sectors_per_fat);
    dbgprintf("Sectors per track: %d\n", sectors_per_track);
    dbgprintf("Total sectors: %d\n", total_sectors);
    // dbgprintf("Partition size: %d MiB\n", partition_size / 1024 / 1024);

    disk_stream_close(stream);
}

#endif
