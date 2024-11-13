#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <file.h>

struct file_system *fat16_init(void);
__attribute__((nonnull)) void fat16_print_partition_stats(const struct disk *disk);
__attribute__((nonnull)) int get_fs_root_directory(const struct disk *disk, struct file_directory *directory);
__attribute__((nonnull)) int fat16_get_subdirectory(struct disk *disk, const char path[static 1],
                                                    struct file_directory *directory);