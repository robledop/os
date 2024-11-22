#pragma once

#ifndef __KERNEL__
#error "This is a kernel header, and should not be included in userspace"
#endif

#include <inode.h>
#include <stdint.h>

#define MAX_MOUNT_POINTS 10

typedef unsigned int FS_ITEM_TYPE;
#define FS_ITEM_TYPE_DIRECTORY 0
#define FS_ITEM_TYPE_FILE 1

struct disk;

typedef int (*FS_RESOLVE_FUNCTION)(struct disk *disk);
typedef int (*FS_GET_ROOT_DIRECTORY_FUNCTION)(const struct disk *disk, struct dir_entries *directory);

struct file_system {
    // file_system should return zero from resolve if the disk is using its file system
    FS_RESOLVE_FUNCTION resolve;
    FS_GET_ROOT_DIRECTORY_FUNCTION get_root_directory;

    char name[20];
    enum FS_TYPE type;
};

struct file_descriptor {
    enum FS_TYPE fs_type;
    int index;
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

void vfs_init(void);
void vfs_add_mount_point(const char *prefix, uint32_t disk_number, struct inode *inode);
int vfs_open(const char path[static 1], const char mode[static 1]);
__attribute__((nonnull)) int vfs_read(void *ptr, uint32_t size, uint32_t nmemb, int fd);
__attribute__((nonnull)) int vfs_write(int fd, const char *buffer, size_t size);
int vfs_seek(int fd, int offset, FILE_SEEK_MODE whence);
__attribute__((nonnull)) int vfs_stat(int fd, struct file_stat *stat);
int vfs_close(int fd);
__attribute__((nonnull)) void vfs_insert_file_system(struct file_system *filesystem);
__attribute__((nonnull)) struct file_system *vfs_resolve(struct disk *disk);
int vfs_open_dir(const char path[static 1], struct dir_entries **directory);
int vfs_get_non_root_mount_point_count();
int vfs_find_mount_point(const char *prefix);
struct mount_point *vfs_get_mount_point(int index);
