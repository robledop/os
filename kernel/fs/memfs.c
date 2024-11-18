#include <kernel_heap.h>
#include <memfs.h>
#include <memory.h>
#include <status.h>
#include <string.h>

#define INITIAL_CAPACITY 10

static uint32_t next_inode_number = 1;

struct inode_operations file_inode_ops = {
    .open    = memfs_open,
    .read    = memfs_read,
    .write   = memfs_write,
    .release = memfs_release,
};

struct inode_operations directory_inode_ops = {
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

struct inode *memfs_create_inode(enum inode_type type)
{
    auto new_inode = (struct inode *)kzalloc(sizeof(struct inode));
    if (!new_inode) {
        return nullptr;
    }

    new_inode->inode_number = next_inode_number++;
    new_inode->type         = type;
    new_inode->size         = 0;

    switch (type) {
    case INODE_FILE:
        new_inode->ops  = &file_inode_ops;
        new_inode->data = nullptr;
        break;
    case INODE_DIRECTORY:
        new_inode->ops  = &directory_inode_ops;
        new_inode->data = create_directory_entries();
        break;
    default:
        kfree(new_inode);
        return nullptr;
    }

    return new_inode;
}

struct inode *memfs_create_device_inode(struct inode_operations *ops)
{
    auto new_inode = (struct inode *)kzalloc(sizeof(struct inode));
    if (!new_inode) {
        return nullptr;
    }

    new_inode->inode_number = next_inode_number++;
    new_inode->type         = INODE_DEVICE;
    new_inode->size         = 0;
    new_inode->ops          = ops;
    new_inode->data         = nullptr;

    return new_inode;
}

int memfs_add_entry_to_directory(struct inode *dir_inode, struct inode *entry, const char *name)
{
    if (dir_inode->type != INODE_DIRECTORY) {
        return -ENOTDIR;
    }

    auto new_entry = (struct dir_entry *)kzalloc(sizeof(struct dir_entry));
    if (!new_entry) {
        return -ENOMEM;
    }

    strncpy(new_entry->name, name, MAX_NAME_LEN);
    new_entry->inode = entry;

    auto entries = (struct dir_entries *)dir_inode->data;
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

int memfs_read(struct dir_entry *file, char *buffer, size_t size, off_t offset)
{
    // Implement reading from file->inode->data


    return 0;
}

int memfs_write(struct dir_entry *file, const char *buffer, size_t size, off_t offset)
{
    // Implement writing to file->inode->data

    return 0;
}

int memfs_open(struct inode *inode, struct dir_entry *file)
{
    // Implement any setup needed when a file is opened

    return 0;
}

int memfs_release(struct inode *inode, struct dir_entry *file)
{
    // Clean up resources when a file is closed
    return 0;
}

int memfs_create_file(struct inode *dir, const char *name)
{
    struct inode *file = memfs_create_inode(INODE_FILE);
    return memfs_add_entry_to_directory(dir, file, name);
}

int memfs_create_device(struct inode *dir, const char *name, struct inode_operations *ops)
{
    struct inode *device = memfs_create_device_inode(ops);
    return memfs_add_entry_to_directory(dir, device, name);
}

int memfs_lookup(const struct inode *dir, const char *name, struct inode **result)
{
    if (dir->type != INODE_DIRECTORY) {
        return -ENOTDIR;
    }

    auto const entries = (struct dir_entries *)dir->data;
    for (size_t i = 0; i < entries->count; ++i) {
        if (strncmp(entries->entries[i]->name, name, MAX_NAME_LEN) == 0) {
            *result = entries->entries[i]->inode;
            return 0;
        }
    }

    *result = nullptr;
    return -ENOENT;
}

int memfs_mkdir(struct inode *dir, const char *name)
{
    struct inode *subdir = memfs_create_inode(INODE_DIRECTORY);
    return memfs_add_entry_to_directory(dir, subdir, name);

    return 0;
}

// struct dir_entry *memfs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
// {
//     return nullptr;
// }
