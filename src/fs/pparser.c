#include "pparser.h"
#include "string.h"
#include "config.h"
#include "kheap.h"
#include "status.h"

static int path_valid_format(const char *path)
{
    size_t len = strnlen(path, MAX_PATH_LENGTH);

    return len >= 3 && isdigit(path[0]) && memcmp((void *)&path[1], ":/", 2) == 0;
}

static int get_drive_by_path(const char **path)
{
    if (!path_valid_format(*path))
    {
        return -EBADPATH;
    }

    int drive_number = tonumericdigit(*path[0]);

    // Skip drive number and colon and slash
    *path += 3;

    return drive_number;
}

static struct path_root *create_root(int drive_number)
{
    struct path_root *root = kzalloc(sizeof(struct path_root));
    root->drive_number = drive_number;
    root->first = 0;

    return root;
}

static const char *get_path_part(const char **path)
{
    char *path_part = kzalloc(MAX_PATH_LENGTH);
    int i = 0;
    while (**path != '/' && **path != 0x00)
    {
        path_part[i] = **path;
        *path += 1;
        i++;
    }

    if (**path == '/')
    {
        // Skip slash
        *path += 1;
    }

    if (i == 0)
    {
        kfree(path_part);
        path_part = 0;
    }

    return path_part;
}

struct path_part *parse_path_part(struct path_part *last_part, const char **path)
{
    const char *path_part_str = get_path_part(path);
    if (path_part_str == 0)
    {
        return 0;
    }

    struct path_part *path_part = kzalloc(sizeof(struct path_part));
    path_part->part = path_part_str;
    path_part->next = 0;

    if (last_part)
    {
        last_part->next = path_part;
    }

    return path_part;
}

void pathparser_free(struct path_root *root)
{
    struct path_part *part = root->first;
    while (part)
    {
        struct path_part *next = part->next;
        kfree((void *)part->part);
        kfree(part);
        part = next;
    }

    kfree(root);
}

struct path_root *pathparser_parse(const char *path, const char *current_directory_path)
{
    int res = 0;
    const char *tmp_path = path;
    struct path_root *root = 0;

    if (strlen(path) > MAX_PATH_LENGTH)
    {
        goto out;
    }

    res = get_drive_by_path(&tmp_path);
    if (res < 0)
    {
        goto out;
    }

    root = create_root(res);
    if (!root)
    {
        goto out;
    }

    struct path_part *first_part = parse_path_part(NULL, &tmp_path);
    if (!first_part)
    {
        goto out;
    }

    root->first = first_part;
    struct path_part *part = parse_path_part(first_part, &tmp_path);

    while (part)
    {
        part = parse_path_part(part, &tmp_path);
    }

out:
    return root;
}