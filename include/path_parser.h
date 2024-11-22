#pragma once
#include <stdint.h>

struct path_root {
    int drive_number;
    uint32_t inode_number;
    struct path_part *first;
};

struct path_part {
    const char *name;
    struct path_part *next;
};

struct path_root *path_parser_parse(const char path[static 1], const char *current_directory_path);
__attribute__((nonnull)) void path_parser_free(struct path_root *root);
