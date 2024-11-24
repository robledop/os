#include <debug.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memfs.h>
#include <memory.h>
#include <root_inode.h>
#include <status.h>
#include <string.h>

#define INITIAL_CAPACITY 10

static uint32_t next_inode_number = 1;

struct inode_operations memfs_directory_inode_ops = {
    .open          = memfs_open,
    .read          = memfs_read,
    .write         = memfs_write,
    .close         = memfs_close,
    .stat          = memfs_stat,
    .create_file   = memfs_create_file,
    .create_device = memfs_create_device,
    .lookup        = memfs_lookup,
    .mkdir         = memfs_mkdir,
};

static void *create_directory_entries()
{
    auto const entries = (struct dir_entries *)kzalloc(sizeof(struct dir_entries));
    if (!entries) {
        return nullptr;
    }

    entries->count    = 0;
    entries->capacity = INITIAL_CAPACITY;
    entries->entries  = (struct dir_entry **)kzalloc(entries->capacity * sizeof(struct dir_entry *));
    if (!entries->entries) {
        kfree(entries);
        return nullptr;
    }

    return entries;
}

struct inode *memfs_create_inode(const enum inode_type type, struct inode_operations *ops)
{
    auto const new_inode = (struct inode *)kzalloc(sizeof(struct inode));
    if (!new_inode) {
        return nullptr;
    }

    new_inode->inode_number = next_inode_number++;
    new_inode->type         = type;
    new_inode->size         = 0;
    new_inode->fs_type      = FS_TYPE_RAMFS;
    new_inode->ops          = ops;

    return new_inode;
}

int memfs_add_entry_to_directory(struct inode *dir_inode, struct inode *entry, const char *name)
{
    if (dir_inode->type != INODE_DIRECTORY) {
        return -ENOTDIR;
    }

    if (dir_inode->dir_magic != DIR_MAGIC) {
        dir_inode->data      = create_directory_entries();
        dir_inode->dir_magic = DIR_MAGIC;
    }

    auto const new_entry = (struct dir_entry *)kzalloc(sizeof(struct dir_entry));
    if (!new_entry) {
        return -ENOMEM;
    }

    strncpy(new_entry->name, name, MAX_NAME_LEN);
    new_entry->inode = entry;

    auto const entries = (struct dir_entries *)dir_inode->data;
    if (entries->count >= entries->capacity) {
        const size_t new_capacity    = entries->capacity * 2;
        struct dir_entry **new_array = krealloc(entries->entries, new_capacity * sizeof(struct dir_entry *));
        if (!new_array) {
            return -ENOMEM;
        }
        entries->entries  = new_array;
        entries->capacity = new_capacity;
    }

    entries->entries[entries->count++] = new_entry;

    dir_inode->size += sizeof(struct dir_entry);

    return 0;
}

int memfs_stat(void *descriptor, struct file_stat *stat)
{
    auto const desc = (struct file_descriptor *)descriptor;
    return desc->inode->ops->stat(descriptor, stat);
}

int memfs_read(const void *descriptor, size_t size, off_t offset, char *out)
{
    panic("Not implemented");
    return 0;
}

int memfs_write(void *descriptor, const char *buffer, size_t size)
{
    panic("Not implemented");
    return 0;
}

void *memfs_open(const struct path_root *path_root, const FILE_MODE mode)
{
    struct inode *dir = nullptr;
    root_inode_lookup(path_root->first->name, &dir);
    struct inode *file;
    memfs_lookup(dir, path_root->first->next->name, &file);
    return file->ops->open(path_root, mode);
}

int memfs_close(void *descriptor)
{
    panic("Not implemented");
    return 0;
}

int memfs_create_file(struct inode *dir, const char *name, struct inode_operations *ops)
{
    struct inode *file = memfs_create_inode(INODE_FILE, ops);
    return memfs_add_entry_to_directory(dir, file, name);
}

int memfs_create_device(struct inode *dir, const char *name, struct inode_operations *ops)
{
    struct inode *device = memfs_create_inode(INODE_DEVICE, ops);
    return memfs_add_entry_to_directory(dir, device, name);
}

int memfs_lookup(const struct inode *dir, const char *name, struct inode **result)
{
    if (dir->type != INODE_DIRECTORY) {
        return -ENOTDIR;
    }

    auto const entries = (struct dir_entries *)dir->data;
    ASSERT(entries != nullptr);
    if (entries->entries[0] == nullptr) {
        return -ENOENT;
    }

    for (size_t i = 0; i < entries->count; ++i) {
        if (strncmp(entries->entries[i]->name, name, MAX_NAME_LEN) == 0) {
            *result = entries->entries[i]->inode;
            return ALL_OK;
        }
    }

    *result = nullptr;
    return -ENOENT;
}

int memfs_mkdir(struct inode *dir, const char *name, struct inode_operations *ops)
{
    struct inode *subdir = memfs_create_inode(INODE_DIRECTORY, ops);
    return memfs_add_entry_to_directory(dir, subdir, name);
}