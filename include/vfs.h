#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <config.h>
#include <dirent.h>
#include <disk.h>
#include <path_parser.h>
#include <stdint.h>
#include <sys/stat.h>
#include <task.h>
#include <time.h>

#define MAX_MOUNT_POINTS 10

typedef unsigned int FS_ITEM_TYPE;
#define FS_ITEM_TYPE_DIRECTORY 0
#define FS_ITEM_TYPE_FILE 1

#define MAX_NAME_LEN 256
#define DIR_MAGIC 0x1eaadf71

enum FS_TYPE {
    FS_TYPE_FAT16,
    FS_TYPE_RAMFS,
};

enum INODE_TYPE {
    INODE_FILE,
    INODE_DIRECTORY,
    INODE_DEVICE,
};

struct dir_entries;


typedef int (*FS_RESOLVE_FUNCTION)(struct disk *disk);
typedef int (*FS_GET_ROOT_DIRECTORY_FUNCTION)(const struct disk *disk, struct dir_entries *directory);
typedef int (*FS_MKDIR_FUNCTION)(const char *path);

struct file_system {
    // file_system should return zero from resolve if the disk is using its file system
    FS_RESOLVE_FUNCTION resolve;
    FS_GET_ROOT_DIRECTORY_FUNCTION get_root_directory;
    struct inode_operations *ops;

    char name[20];
    enum FS_TYPE type;
};

struct file {
    char path[MAX_PATH_LENGTH];
    enum FS_TYPE fs_type;
    enum INODE_TYPE type;
    int index;
    off_t offset;
    uint32_t size;
    struct file_system *fs;
    struct inode *inode;
    struct disk *disk;
    void *fs_data;
};

struct mount_point {
    struct file_system *fs;
    char *prefix;
    uint32_t disk;
    struct inode *inode;
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


// In-memory representation of a directory entry
struct dir_entry {
    struct inode *inode;
    uint8_t file_type;       // File type
    uint8_t name_length;     // Length of the name
    char name[NAME_MAX + 1]; // Null-terminated filename
};

struct dir_entries {
    size_t count;
    size_t capacity;
    struct dir_entry **entries;
};


// enum FILE_MODE { FILE_MODE_READ, FILE_MODE_WRITE, FILE_MODE_APPEND, FILE_MODE_INVALID };

struct inode_operations {
    void *(*open)(const struct path_root *path_root, FILE_MODE mode, enum INODE_TYPE *type_out, uint32_t *size_out);
    int (*read)(const void *descriptor, size_t size, off_t nmemb, char *out);
    int (*write)(const void *descriptor, const char *buffer, size_t size);
    int (*seek)(void *descriptor, uint32_t offset, enum FILE_SEEK_MODE seek_mode);
    int (*stat)(void *descriptor, struct stat *stat);
    int (*close)(void *descriptor);
    int (*ioctl)(void *descriptor, int request, void *arg);

    int (*create_file)(struct inode *dir, const char *name, struct inode_operations *ops);
    int (*create_device)(struct inode *dir, const char *name, struct inode_operations *ops);

    int (*lookup)(const struct inode *dir, const char *name, struct inode **result);
    int (*mkdir)(const char *name);

    // typedef int (*FS_GET_SUB_DIRECTORY_FUNCTION)(const char *path, struct dir_entries *directory);
    int (*get_sub_directory)(const struct path_root *path_root, struct dir_entries *result);

    int (*read_entry)(struct file *descriptor, struct dir_entry *entry);
};

void vfs_init(void);
void vfs_add_mount_point(const char *prefix, uint32_t disk_number, struct inode *inode);
int vfs_open(const char path[static 1], int mode);
__attribute__((nonnull)) int vfs_read(void *ptr, uint32_t size, uint32_t nmemb, int fd);
__attribute__((nonnull)) int vfs_write(int fd, const char *buffer, size_t size);
int vfs_seek(int fd, int offset, enum FILE_SEEK_MODE whence);
__attribute__((nonnull)) int vfs_stat(int fd, struct stat *stat);
int vfs_close(int fd);
__attribute__((nonnull)) void vfs_insert_file_system(struct file_system *filesystem);
__attribute__((nonnull)) struct file_system *vfs_resolve(struct disk *disk);
int vfs_getdents(const uint32_t fd, void *buffer, int count);
int vfs_get_non_root_mount_point_count();
int vfs_find_mount_point(const char *prefix);
struct mount_point *vfs_get_mount_point(int index);
int vfs_mkdir(const char *path);
int vfs_lseek(int fd, int offset, enum FILE_SEEK_MODE whence);
