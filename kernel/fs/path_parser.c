#include <config.h>
#include <kernel_heap.h>
#include <path_parser.h>
#include <serial.h>
#include <status.h>
#include <string.h>
#include <vfs.h>
#include <vga_buffer.h>
#ifndef __KERNEL__
#include <stdlib.h>
#endif

static int path_valid_format(const char *path)
{
    const size_t len = strnlen(path, MAX_PATH_LENGTH);
    return len >= 3 && isdigit(path[0]) && memcmp((void *)&path[1], ":/", 2) == 0;
}

static int get_drive_by_path(char **path)
{
    dbgprintf("Getting drive by path %s\n", *path);
    if (!path_valid_format(*path)) {
        warningf("Invalid path format\n");
        return -EBADPATH;
    }

    const int drive_number = tonumericdigit(*path[0]);

    // Skip drive number and colon and slash
    *path += 3;

    return drive_number;
}

static struct path_root *create_root(const int drive_number, const uint32_t inode_number)
{
#ifdef __KERNEL__
    struct path_root *root = kzalloc(sizeof(struct path_root));
#else
    struct path_root *root = calloc(sizeof(struct path_root), 1);
#endif

    root->drive_number = drive_number;
    root->inode_number = inode_number;
    root->first        = nullptr;

    return root;
}

static const char *get_path_part(const char **path)
{
    if (starts_with("/", *path)) {
        *path += 1;
    }

#ifdef __KERNEL__
    char *path_part = kzalloc(MAX_PATH_LENGTH);
#else
    char *path_part = calloc(MAX_PATH_LENGTH, 1);
#endif
    int i = 0;
    while (**path != '/' && **path != 0x00) {
        path_part[i] = **path;
        *path += 1;
        i++;
    }

    if (**path == '/') {
        // Skip slash
        *path += 1;
    }

    if (i == 0) {
#ifdef __KERNEL__
        kfree(path_part);
#else
        free(path_part);
#endif
        path_part = nullptr;
    }

    return path_part;
}

struct path_part *parse_path_part(struct path_part *last_part, const char **path)
{
    const char *path_part_str = get_path_part(path);
    if (path_part_str == nullptr) {
        return nullptr;
    }

#ifdef __KERNEL__
    struct path_part *path_part = kzalloc(sizeof(struct path_part));
#else
    struct path_part *path_part = calloc(sizeof(struct path_part), 1);
#endif
    if (path_part == nullptr) {
#ifdef __KERNEL__
        kfree((void *)path_part_str);
#else
        free((void *), path_part_str);
#endif

        warningf("Failed to allocate path part\n");
        return nullptr;
    }
    path_part->name = path_part_str;
    path_part->next = nullptr;

    if (last_part) {
        last_part->next = path_part;
    }

    return path_part;
}

void path_parser_free(struct path_root *root)
{
    struct path_part *part = root->first;
    while (part) {
        struct path_part *next = part->next;
#ifdef __KERNEL__
        kfree((void *)part->name);
        kfree(part);
#else
        free((void *)part->part);
        free(part);
#endif

        part = next;
    }

#ifdef __KERNEL__
    kfree(root);
#else
    free(root);
#endif
}

static struct mount_point *determine_mount_point(const char *path)
{
    char *path_dup = strdup(path);
    if (strncmp("/", path_dup, 1) == 0) {
        path_dup++;
    }
    int mount_point_index;
    char *beginning = strtok(path_dup, "/");
    if (!beginning) {
        mount_point_index = vfs_find_mount_point("/");
    } else {
        char s[strlen(beginning) + 1] = {};
        strcat(s, "/");
        strcat(s, beginning);
        mount_point_index = vfs_find_mount_point(s);
        if (mount_point_index < 0) {
            mount_point_index = vfs_find_mount_point("/");
        }
    }
    struct mount_point *mount_point = vfs_get_mount_point(mount_point_index);

    kfree(path_dup);
    return mount_point;
}

struct path_root *path_parser_parse(const char path[static 1], const char *current_directory_path)
{
    dbgprintf("Parsing path %s\n", path);
    struct path_root *root = nullptr;

    if (strlen(path) > MAX_PATH_LENGTH) {
        warningf("Path too long\n");
        goto out;
    }

    struct mount_point *mount_point = determine_mount_point(path);

    if (mount_point->inode == nullptr) {
        root = create_root((int)mount_point->disk, -1);
    } else {
        root = create_root(-1, mount_point->inode->inode_number);
    }

    if (!root) {
        warningf("Failed to create root\n");
        goto out;
    }

    struct path_part *first_part = parse_path_part(nullptr, &path);

    if (!first_part) {
        warningf("Failed to parse first part\n");
        kfree(root);
        goto out;
    }

    root->first = first_part;

    struct path_part *part = parse_path_part(first_part, &path);

    while (part) {
        part = parse_path_part(part, &path);
    }

out:
    dbgprintf("Returning path %s\n", root ? "root" : "NULL");
    return root;
}