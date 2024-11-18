#pragma once

#include <posix.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_NAME_LEN 256

enum inode_type {
    INODE_FILE,
    INODE_DIRECTORY,
    INODE_DEVICE,
};

struct inode {
    uint32_t inode_number;
    enum inode_type type;
    uint32_t size;
    struct inode_operations *ops;
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

struct inode_operations {
    int (*open)(struct inode *file, struct dir_entry *entry);
    int (*read)(struct dir_entry *file, char *buffer, size_t size, off_t offset);
    int (*write)(struct dir_entry *file, const char *buffer, size_t size, off_t offset);
    int (*seek)(struct dir_entry *file, off_t offset, int whence);
    int (*stat)(struct dir_entry *file, struct file_stat *stat);
    int (*release)(struct inode *inode, struct dir_entry *file);
    int (*ioctl)(struct inode *inode, int request, void *arg);

    int (*create_file)(struct inode *dir, const char *name);
    int (*create_device)(struct inode *dir, const char *name, struct inode_operations *ops);

    int (*lookup)(const struct inode *dir, const char *name, struct inode **result);
    int (*mkdir)(struct inode *dir, const char *name);
};
