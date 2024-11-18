#pragma once

#ifndef __KERNEL__
#error "This is a kernel header file. It should not be included in user-space."
#endif

#include <inode.h>
#include <posix.h>
#include <stddef.h>
#include <vfs.h>

struct inode *memfs_create_inode(enum inode_type type);
int memfs_read(struct dir_entry *file, char *buffer, size_t size, off_t offset);
int memfs_write(struct dir_entry *file, const char *buffer, size_t size, off_t offset);
int memfs_open(struct inode *inode, struct dir_entry *file);
int memfs_release(struct inode *inode, struct dir_entry *file);
int memfs_create_file(struct inode *dir, const char *name);
int memfs_create_device(struct inode *dir, const char *name, struct inode_operations *ops);
int memfs_lookup(const struct inode *dir, const char *name, struct inode **result);
int memfs_mkdir(struct inode *dir, const char *name);
int memfs_add_entry_to_directory(struct inode *dir_inode, struct inode *entry, const char *name);
// struct dir_entry *memfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);
