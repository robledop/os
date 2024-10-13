#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "status.h"

void print_results(struct file_directory *directory);

int main(int argc, char **argv)
{
    struct file_directory *directory = malloc(sizeof(struct file_directory));

    int res = 0;
    if (argv[1] == NULL)
    {
        res = opendir(directory, "0:/");
    }
    else
    {
        res = opendir(directory, argv[1]);
    }

    switch (res)
    {
    case ALL_OK:
        print_results(directory);
        break;
    case -ENOENT:
        printf("\nNo such file or directory");
        return 0;
    case -EBADPATH:
        printf("\nInvalid path");
        return 0;
    default:
        printf("\nError: %d", res);
        return 0;
    }

    return 0;
}

void print_results(struct file_directory *directory)
{
    printf("\nEntries in directory: %d", directory->entry_count);
    for (size_t i = 0; i < directory->entry_count; i++)
    {
        struct directory_entry entry;
        readdir(directory, &entry, i);
        if (entry.is_long_name)
        {
            continue;
        }
        if (strlen(entry.ext) > 0)
        {
            printf("\n%s.%s - size: %d bytes, dir: %d, ro: %d, h: %d, s: %d, v: %d",
                   entry.name,
                   entry.ext,
                   entry.size,
                   entry.is_directory,
                   entry.is_read_only,
                   entry.is_hidden,
                   entry.is_system,
                   entry.is_volume_label);
        }
        else
        {
            printf("\n%s - size: %d bytes, dir: %d, ro: %d, h: %d, s: %d, v: %d",
                   entry.name,
                   entry.size,
                   entry.is_directory,
                   entry.is_read_only,
                   entry.is_hidden,
                   entry.is_system,
                   entry.is_volume_label);
        }
    }
}