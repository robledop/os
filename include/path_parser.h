#pragma once
#include <stddef.h>
#include <stdint.h>

struct path_root {
    int drive_number;
    // uint32_t inode_number;
    struct path_part *first;
};

struct path_part {
    const char *name;
    struct path_part *next;
};

struct path_root *path_parser_parse(const char path[static 1], const char *current_directory_path);
struct path_part *path_parser_get_last_part(const struct path_root *root);
__attribute__((nonnull)) void path_parser_free(struct path_root *root);
int path_parser_unparse(const struct path_root *root, char *buffer, size_t buffer_size);
