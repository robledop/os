#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <vfs.h>

struct fat_file_descriptor {
    struct fat_item *item;
    uint32_t position;
    struct disk *disk;
};


struct file_system *fat16_init(void);
__attribute__((nonnull)) void fat16_print_partition_stats(const struct disk *disk);
int fat16_get_vfs_root_directory(const struct disk *disk, struct dir_entries *directory);
__attribute__((nonnull)) int fat16_get_directory_entries(const struct path_root *root_path,
                                                         struct dir_entries *dir_entries);
