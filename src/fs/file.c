#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "terminal/terminal.h"
#include "fs/fat/fat16.h"
#include "string/string.h"
#include "disk/disk.h"
#include "kernel/kernel.h"

struct file_system *file_systems[MAX_FILE_SYSTEMS];
struct file_descriptor *file_descriptors[MAX_FILE_DESCRIPTORS];

static struct file_system **fs_get_free_file_system()
{
    int i = 0;
    for (i = 0; i < MAX_FILE_SYSTEMS; i++)
    {
        if (file_systems[i] == 0)
        {
            return &file_systems[i];
        }
    }

    return 0;
}

void fs_insert_file_system(struct file_system *filesystem)
{
    struct file_system **fs;
    fs = fs_get_free_file_system();
    if (!fs)
    {
        print("Problem inserting filesystem");
        while (1)
        {
        }
    }

    *fs = filesystem;
}

static void fs_static_load()
{
    fs_insert_file_system(fat16_init());
}

void fs_load()
{
    memset(file_systems, 0, sizeof(file_systems));
    fs_static_load();
}

void fs_init()
{
    // print("Initializing file system\n");
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static int file_new_descriptor(struct file_descriptor **desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor *desc = (struct file_descriptor *)kzalloc(sizeof(struct file_descriptor));
            // if (desc == 0)
            // {
            //     res = -ENOMEM;
            //     break;
            // }

            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

static struct file_descriptor *file_get_descriptor(int index)
{
    if (index < 1 || index > MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    return file_descriptors[index - 1];
}

struct file_system *fs_resolve(struct disk *disk)
{
    for (int i = 0; i < MAX_FILE_SYSTEMS; i++)
    {
        if (file_systems[i] != 0 && file_systems[i]->resolve(disk) == 0)
        {
            return file_systems[i];
        }
    }

    return 0;
}

FILE_MODE file_get_mode_from_string(const char *mode)
{
    if (strncmp(mode, "r", 1) == 0)
    {
        return FILE_MODE_READ;
    }
    else if (strncmp(mode, "w", 1) == 0)
    {
        return FILE_MODE_WRITE;
    }
    else if (strncmp(mode, "a", 1) == 0)
    {
        return FILE_MODE_APPEND;
    }

    return FILE_MODE_INVALID;
}

// Open a file
// Returns a file descriptor
// fopen returns 0 on error
// Parameters:
// - path: the path to the file
// - mode: the mode to open the file in
//  - r: read
//  - w: write
//  - a: append
int fopen(const char *path, const char *mode)
{
    print("fopen: ");
    print(path);
    print(", mode: ");
    print_line(mode);

    int res = 0;

    struct path_root *root_path = pathparser_parse(path, NULL);
    if (!root_path)
    {
        print_line("Failed to parse path");
        res = -EBADPATH;
        goto out;
    }

    if (!root_path->first)
    {
        print_line("Path does not contain a file");
        res = -EBADPATH;
        goto out;
    }

    struct disk *disk = disk_get(root_path->drive_number);
    if (!disk)
    {
        print_line("Failed to get disk");
        res = -EIO;
        goto out;
    }

    if (disk->fs == NULL)
    {
        print_line("Disk has no file system");
        res = -EIO;
        goto out;
    }

    FILE_MODE file_mode = file_get_mode_from_string(mode);
    if (file_mode == FILE_MODE_INVALID)
    {
        print_line("Invalid file mode");
        res = -EINVARG;
        goto out;
    }

    void *descriptor_private_data = disk->fs->open(disk, root_path->first, file_mode);
    if (ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor *desc = NULL;
    res = file_new_descriptor(&desc);
    if (ISERR(res))
    {
        print_line("Failed to create file descriptor");
        goto out;
    }

    desc->fs = disk->fs;
    desc->fs_data = descriptor_private_data;
    desc->disk = disk;

    res = desc->index;

out:
    // fopen returns 0 on error
    if (ISERR(res))
    {
        res = 0;
    }
    return res;
}