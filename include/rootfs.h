#pragma once

#include <inode.h>

void root_inode_init(void);
struct inode *root_inode_lookup(const char *name);
int root_inode_mkdir(const char *name, struct inode_operations* ops);
struct dir_entries *root_inode_get_root_directory(void);
