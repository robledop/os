#pragma once

#ifndef __KERNEL__
#error "This is a kernel header file. It should not be included in user-space."
#endif

#include <path_parser.h>
#include <posix.h>
#include <stddef.h>
#include <vfs.h>

struct inode *memfs_create_inode(enum INODE_TYPE type, struct inode_operations *ops);
int memfs_read(const void *descriptor, size_t size, off_t offset, char *out);
int memfs_write(const void *descriptor, const char *buffer, size_t size);
void *memfs_open(const struct path_root *path_root, FILE_MODE mode, enum INODE_TYPE *type_out, uint32_t *size_out);
int memfs_read_entry(struct file *descriptor, struct dir_entry *entry);
int memfs_close(void *descriptor);
int memfs_create_file(struct inode *dir, const char *name, struct inode_operations *ops);
int memfs_create_device(struct inode *dir, const char *name, struct inode_operations *ops);
int memfs_lookup(const struct inode *dir, const char *name, struct inode **result);
int memfs_mkdir(struct inode *dir, const char *name, struct inode_operations *ops);
int memfs_add_entry_to_directory(struct inode *dir_inode, struct inode *entry, const char *name);
int memfs_stat(void *descriptor, struct stat *stat);
int memfs_load_directory(struct inode *dir, const struct path_root *root_path);
void *create_directory_entries();
