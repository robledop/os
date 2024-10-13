#ifndef FAT16_H
#define FAT16_H
#include "file.h"

struct file_system *fat16_init();
void fat16_print_partition_stats(struct disk *disk);
int get_fs_root_directory(struct disk *disk, struct file_directory *directory);
int get_subdirectory(struct disk *disk, const char *path, struct file_directory *directory);
#endif