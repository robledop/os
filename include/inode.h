#pragma once

#include <config.h>
#include <path_parser.h>
#include <posix.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define MAX_NAME_LEN 256
#define DIR_MAGIC 0x1eaadf71

typedef unsigned int FILE_STAT_FLAGS;
enum { FILE_STAT_IS_READ_ONLY = 0b00000001 };

struct file_stat {
    FILE_STAT_FLAGS flags;
    uint32_t size;
};

enum INODE_TYPE {
    INODE_FILE,
    INODE_DIRECTORY,
    INODE_DEVICE,
};

enum FS_TYPE {
    FS_TYPE_FAT16,
    FS_TYPE_RAMFS,
};

struct inode {
    uint32_t inode_number;
    char path[MAX_PATH_LENGTH];
    enum INODE_TYPE type;
    enum FS_TYPE fs_type;
    uint32_t size;
    time_t atime; // Last access time
    time_t mtime; // Last modification time
    time_t ctime; // Creation time
    bool is_read_only : 1;
    bool is_hidden    : 1;
    bool is_system    : 1;
    bool is_archive   : 1;
    struct inode_operations *ops;
    uint32_t dir_magic; // Used to check if the directory has been initialized
    void *data;
};

struct dir_entry {
    char name[MAX_NAME_LEN];
    struct inode *inode;
};

struct dir_entries {
    size_t count;
    size_t capacity;
    struct dir_entry **entries;
};

typedef unsigned int FILE_SEEK_MODE;
enum { SEEK_SET, SEEK_CURRENT, SEEK_END };

typedef unsigned int FILE_MODE;
enum { FILE_MODE_READ, FILE_MODE_WRITE, FILE_MODE_APPEND, FILE_MODE_INVALID };

struct inode_operations {
    void *(*open)(const struct path_root *path_root, FILE_MODE mode);
    int (*read)(const void *descriptor, size_t size, off_t nmemb, char *out);
    int (*write)(const void *descriptor, const char *buffer, size_t size);
    int (*seek)(void *descriptor, uint32_t offset, FILE_SEEK_MODE seek_mode);
    int (*stat)(void *descriptor, struct file_stat *stat);
    int (*close)(void *descriptor);
    int (*ioctl)(void *descriptor, int request, void *arg);

    int (*create_file)(struct inode *dir, const char *name, struct inode_operations *ops);
    int (*create_device)(struct inode *dir, const char *name, struct inode_operations *ops);

    int (*lookup)(const struct inode *dir, const char *name, struct inode **result);
    int (*mkdir)(const char *name);

    // typedef int (*FS_GET_SUB_DIRECTORY_FUNCTION)(const char *path, struct dir_entries *directory);
    int (*get_sub_directory)(const struct path_root *path_root, struct dir_entries *result);
};
