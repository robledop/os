#ifndef FAT16_H
#define FAT16_H
#include "file.h"

struct file_system *fat16_init();
void fat16_print_partition_stats(struct disk *disk);
#endif