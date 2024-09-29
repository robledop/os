#include "fat16.h"
#include "status.h"
#include "string/string.h"
#include "terminal/terminal.h"

int fat16_resolve(struct disk *disk);
void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode);

struct file_system fat16_fs = {
    .resolve = fat16_resolve,
    .open = fat16_open,
};

struct file_system *fat16_init()
{
    strncpy(fat16_fs.name, "FAT16", 20);
    print_line(fat16_fs.name);
    return &fat16_fs;
}

int fat16_resolve(struct disk *disk)
{
    print_line("Resolving FAT16");
    return 0;
}

void *fat16_open(struct disk *disk, struct path_part *path, FILE_MODE mode)
{
    print_line("Opening FAT16");
    return 0;
}
