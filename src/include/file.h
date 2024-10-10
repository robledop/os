#ifndef FILE_H
#define FILE_H
#include "path_parser.h"
#include <stdint.h>
#include <stdbool.h>

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


typedef unsigned int FILE_SEEK_MODE;
enum
{
    SEEK_SET,
    SEEK_CURRENT,
    SEEK_END
};

typedef unsigned int FILE_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

typedef unsigned int FILE_STAT_FLAGS;
enum
{
    FILE_STAT_IS_READ_ONLY = 0b00000001
};

struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t size;
};

struct disk;
typedef void *(*FS_OPEN_FUNCTION)(struct disk *disk, struct path_part *path, FILE_MODE mode);
typedef int (*FS_READ_FUNCTION)(struct disk *disk, void *private, uint32_t size, uint32_t nmemb, char *out);
typedef int (*FS_SEEK_FUNCTION)(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode);
typedef int (*FS_CLOSE_FUNCTION)(void *private);
typedef int (*FS_STAT_FUNCTION)(struct disk *disk, void *private, struct file_stat *stat);
typedef int (*FS_RESOLVE_FUNCTION)(struct disk *disk);

/////////////////////////////

typedef struct file_directory (*FS_GET_ROOT_DIRECTORY_FUNCTION)(struct disk *disk);
typedef struct file_directory (*FS_GET_SUB_DIRECTORY_FUNCTION)(struct disk *disk, const char *path);
typedef struct directory_entry (*DIRECTORY_GET_ENTRY)(void *entries, int index);

struct directory_entry
{
    char *name;
    char *ext;
    uint8_t attributes;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t modification_time;
    uint16_t modification_date;
    uint32_t size;
    bool is_directory;
    bool is_read_only;
    bool is_hidden;
    bool is_system;
    bool is_volume_label;
    bool is_long_name;
    bool is_archive;
    bool is_device;
};

struct file_directory
{
    char *name;
    int entry_count;
    void *entries;
    DIRECTORY_GET_ENTRY get_entry;
};
////////////////////////////

struct file_system
{
    // file_system should return zero from resolve if the disk is using its file system
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;

    FS_GET_ROOT_DIRECTORY_FUNCTION get_root_directory;
    FS_GET_SUB_DIRECTORY_FUNCTION get_subdirectory;

    char name[20];
};

struct file_descriptor
{
    int index;
    struct file_system *fs;
    // Private data for internal file descriptor use
    void *fs_data;
    struct disk *disk;
};

void fs_init();
int fopen(const char *path, const char *mode);
int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
int fstat(int fd, struct file_stat *stat);
int fclose(int fd);
void fs_insert_file_system(struct file_system *fs);
struct file_system *fs_resolve(struct disk *disk);
struct file_directory fs_open_dir(const char *name);

#endif