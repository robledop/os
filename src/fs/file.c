#include "file.h"
#include "config.h"
#include "memory.h"
#include "kheap.h"
#include "status.h"
#include "fat16.h"
#include "string.h"
#include "ata.h"
#include "kernel.h"
#include "serial.h"
#include "assert.h"
#include "terminal.h"

/////////////////////
typedef unsigned int FS_ITEM_TYPE;
#define FS_ITEM_TYPE_DIRECTORY 0
#define FS_ITEM_TYPE_FILE 1

struct fs_item
{
    union
    {
        struct fat_directory_entry *item;
        struct fat_directory *directory;
    };
    FS_ITEM_TYPE type;
};
////////////////////

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
        panic("Problem inserting filesystem");
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
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

static void file_free_descriptor(struct file_descriptor *desc)
{
    file_descriptors[desc->index - 1] = 0x00;
    kfree(desc);
}

static int file_new_descriptor(struct file_descriptor **desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor *desc = (struct file_descriptor *)kzalloc(sizeof(struct file_descriptor));
            if (desc == 0)
            {
                res = -ENOMEM;
                break;
            }

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
        if (file_systems[i] != 0)
        {
            ASSERT(file_systems[i]->resolve != 0, "File system does not have resolve function");
            if (file_systems[i]->resolve(disk) == 0)
            {
                return file_systems[i];
            }
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
    int res = 0;

    struct path_root *root_path = pathparser_parse(path, NULL);
    if (!root_path)
    {
        warningf("Failed to parse path\n");
        res = -EBADPATH;
        goto out;
    }

    if (!root_path->first)
    {
        warningf("Path does not contain a file\n");
        res = -EBADPATH;
        goto out;
    }

    struct disk *disk = disk_get(root_path->drive_number);
    if (!disk)
    {
        warningf("Failed to get disk\n");
        res = -EIO;
        goto out;
    }

    if (disk->fs == NULL)
    {
        warningf("Disk has no file system\n");
        res = -EIO;
        goto out;
    }

    FILE_MODE file_mode = file_get_mode_from_string(mode);
    if (file_mode == FILE_MODE_INVALID)
    {
        warningf("Invalid file mode\n");
        res = -EINVARG;
        goto out;
    }

    void *descriptor_private_data = disk->fs->open(disk, root_path->first, file_mode);
    if (ISERR(descriptor_private_data))
    {
        warningf("Failed to open file\n");
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor *desc = NULL;
    res = file_new_descriptor(&desc);
    if (ISERR(res))
    {
        warningf("Failed to create file descriptor\n");
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

int fstat(int fd, struct file_stat *stat)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc)
    {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->fs->stat(desc->disk, desc->fs_data, stat);
}

int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc)
    {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->fs->seek(desc->fs_data, offset, whence);
}

int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd)
{
    if (size == 0 || nmemb == 0 || fd < 1)
    {
        warningf("Invalid arguments\n");
        return -EINVARG;
    }

    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc)
    {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    if (desc->fs == 0 || desc->fs->read == 0)
    {
        warningf("File system does not support read\n");
        return -EINVARG;
    }

    return desc->fs->read(desc->disk, desc->fs_data, size, nmemb, (char *)ptr);
}

int fclose(int fd)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc)
    {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    if (desc->fs == 0 || desc->fs->close == 0)
    {
        warningf("File system does not support close\n");
        return -EINVARG;
    }

    int res = desc->fs->close(desc->fs_data);

    if (res == ALL_OK)
    {
        file_free_descriptor(desc);
    }

    return res;
}

struct file_directory fs_open_dir(const char *name)
{
    struct path_root *root_path = pathparser_parse(name, NULL);

    ASSERT(root_path != 0, "Failed to parse path");

    struct disk *disk = disk_get(root_path->drive_number);

    ASSERT(disk != 0, "Failed to get disk");
    ASSERT(disk->fs != 0, "Disk has no file system");

    if (root_path->first == NULL)
    {
        ASSERT(disk->fs->get_root_directory != 0, "File system does not support getting root directory");
        return disk->fs->get_root_directory(disk);
    }

    ASSERT(disk->fs->get_subdirectory != 0, "File system does not support getting sub directory");
    ASSERT(name != NULL, "Name is null");
    ASSERT(disk != NULL, "Disk is null");

    struct file_directory dir = disk->fs->get_subdirectory(disk, name);

    return dir;
}