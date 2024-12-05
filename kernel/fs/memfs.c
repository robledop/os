#include <assert.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memfs.h>
#include <memory.h>
#include <path_parser.h>
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
    .read_entry    = memfs_read_entry,
};

int memfs_read_entry(struct file *descriptor, struct dir_entry *entry)
{
    const struct inode *dir = descriptor->inode;
    if (dir->type != INODE_DIRECTORY) {
        return -ENOTDIR;
    }

    const struct dir_entries *entries = (struct dir_entries *)dir->data;
    if (entries == nullptr) {
        return -ENOENT;
    }

    if (descriptor->offset >= (off_t)entries->count) {
        return -ENOENT;
    }

    if (entries->entries[0] == nullptr) {
        return -ENOENT;
    }

    *entry = *entries->entries[descriptor->offset++];

    return ALL_OK;
}

void *create_directory_entries()
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

struct inode *memfs_create_inode(const enum INODE_TYPE type, struct inode_operations *ops)
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
    new_entry->inode       = entry;
    new_entry->name_length = strlen(name);

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

int memfs_stat(void *descriptor, struct stat *stat)
{
    auto const desc = (struct file *)descriptor;
    if (desc->type == INODE_DIRECTORY) {
        stat->st_mode                     = S_IFDIR;
        stat->st_ino                      = desc->inode->inode_number;
        const struct dir_entries *entries = desc->inode->data;
        stat->st_size                     = entries->count;
        return ALL_OK;
    }
    stat->st_mode = S_IFCHR;
    return desc->inode->ops->stat(descriptor, stat);
}

int memfs_read(const void *descriptor, size_t size, off_t offset, char *out)
{
    panic("Not implemented");
    return 0;
}

int memfs_write(const void *descriptor, const char *buffer, size_t size)
{
    panic("Not implemented");
    return 0;
}

void *memfs_open(const struct path_root *path_root, const FILE_MODE mode, enum INODE_TYPE *type_out, uint32_t *size_out)
{
    struct inode *dir = nullptr;
    root_inode_lookup(path_root->first->name, &dir);
    // We are opening the directory itself
    if (path_root->first->next == nullptr) {
        *type_out = dir->type;
        *size_out = dir->size;
        return dir;
    }

    // We are opening a file in the directory
    struct inode *file;
    memfs_lookup(dir, path_root->first->next->name, &file);
    return file->ops->open(path_root, mode, type_out, size_out);
}

int memfs_close(void *descriptor)
{
    // panic("Not implemented");
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
    if (entries == nullptr) {
        return -ENOENT;
    }
    if (entries->entries[0] == nullptr) {
        return -ENOENT;
    }

    for (size_t i = 0; i < entries->count; ++i) {
        if (strncmp(entries->entries[i]->name, name, MAX_NAME_LEN) == 0) {
            if (result != nullptr) {
                *result = entries->entries[i]->inode;
            }
            return ALL_OK;
        }
    }

    if (result != nullptr) {
        *result = nullptr;
    }

    return -ENOENT;
}

int memfs_mkdir(struct inode *dir, const char *name, struct inode_operations *ops)
{
    struct inode *subdir = memfs_create_inode(INODE_DIRECTORY, ops);
    return memfs_add_entry_to_directory(dir, subdir, name);
}

int memfs_load_directory(struct inode *dir, const struct path_root *root_path)
{
    ASSERT(dir->ops->get_sub_directory);

    struct dir_entries *sub_dir_entries = kzalloc(sizeof(struct dir_entries));
    const int res                       = dir->ops->get_sub_directory(root_path, sub_dir_entries);
    if (res < 0) {
        return -EBADPATH;
    }

    for (size_t i = 0; i < sub_dir_entries->count; i++) {
        memfs_add_entry_to_directory(dir, sub_dir_entries->entries[i]->inode, sub_dir_entries->entries[i]->name);
    }

    if (dir->dir_magic != DIR_MAGIC) {
        dir->data      = create_directory_entries();
        dir->dir_magic = DIR_MAGIC;
    }


    return res;
}
