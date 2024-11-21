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
typedef int (*FS_GET_SUB_DIRECTORY_FUNCTION)(const char *path, struct dir_entries *directory);

struct file_system {
    // file_system should return zero from resolve if the disk is using its file system
    FS_RESOLVE_FUNCTION resolve;

    FS_GET_ROOT_DIRECTORY_FUNCTION get_root_directory;
    FS_GET_SUB_DIRECTORY_FUNCTION get_subdirectory;

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

void fs_init(void);
void fs_add_mount_point(const char *prefix, uint32_t disk_number, struct inode *inode);
int fopen(const char path[static 1], const char mode[static 1]);
__attribute__((nonnull)) int fread(void *ptr, uint32_t size, uint32_t nmemb, int fd);
__attribute__((nonnull)) int write(int fd, char *buffer, size_t size);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
__attribute__((nonnull)) int fstat(int fd, struct file_stat *stat);
int fclose(int fd);
__attribute__((nonnull)) void fs_insert_file_system(struct file_system *filesystem);
__attribute__((nonnull)) struct file_system *fs_resolve(struct disk *disk);
int fs_open_dir(const char path[static 1], struct dir_entries **directory);
int fs_get_non_root_mount_point_count();
int fs_find_mount_point(const char *prefix);
struct mount_point *fs_get_mount_point(int index);
