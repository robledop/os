#pragma once

#include "file.h"

struct file_system *fat16_init();
void fat16_print_partition_stats(const struct disk *disk);
int get_fs_root_directory(const struct disk *disk, struct file_directory *directory);
int fat16_get_subdirectory(struct disk *disk, const char *path, struct file_directory *directory);