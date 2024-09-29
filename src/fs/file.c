#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "terminal/terminal.h"
#include "fs/fat/fat16.h"
#include "string/string.h"

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
    print("Initializing file system\n");
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

int fopen(const char *path, FILE_MODE mode)
{
    print("fopen: ");
    print(path);
    print(" mode: ");
    print(hex_to_string(mode));

    return -EIO;
}