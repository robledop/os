#pragma once

#include <inode.h>

void rootfs_init(void);
struct inode *rootfs_lookup(const char *name);
int rootfs_mkdir(const char *name);
struct dir_entries *rootfs_get_root_directory(void);
