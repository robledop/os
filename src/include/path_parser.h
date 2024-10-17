#ifndef PPARSER_H
#define PPARSER_H

struct path_root
{
    int drive_number;
    struct path_part *first;
};

struct path_part
{
    const char *part;
    struct path_part *next;
};

struct path_root *path_parser_parse(const char *path, const char *current_directory_path);
void path_parser_free(struct path_root *root);

#endif