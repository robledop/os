#include <config.h>
#include <debug.h>
#include <disk.h>
#include <fat16.h>
#include <kernel.h>
#include <kernel_heap.h>
#include <memfs.h>
#include <memory.h>
#include <root_inode.h>
#include <serial.h>
#include <status.h>
#include <string.h>
#include <vfs.h>


struct mount_point *mount_points[MAX_MOUNT_POINTS];

struct file_system *file_systems[MAX_FILE_SYSTEMS];
struct file_descriptor *file_descriptors[MAX_FILE_DESCRIPTORS];

static struct file_system **fs_get_free_file_system()
{
    int i = 0;
    for (i = 0; i < MAX_FILE_SYSTEMS; i++) {
        if (file_systems[i] == nullptr) {
            return &file_systems[i];
        }
    }

    return nullptr;
}

void vfs_insert_file_system(struct file_system *filesystem)
{
    struct file_system **fs = fs_get_free_file_system();
    if (!fs) {
        panic("Problem inserting filesystem");
        return;
    }

    *fs = filesystem;
}

void fs_load()
{
    memset(file_systems, 0, sizeof(file_systems));
    vfs_insert_file_system(fat16_init());
}

void vfs_init()
{
    memset(mount_points, 0, sizeof(mount_points));
    memset(file_descriptors, 0, sizeof(file_descriptors));

    fs_load();
}

int vfs_find_mount_point(const char *prefix)
{
    if (prefix == nullptr) {
        return -1;
    }

    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i] != nullptr) {
            if (mount_points[i]->prefix &&
                strncmp(mount_points[i]->prefix, prefix, strlen(mount_points[i]->prefix)) == 0 &&
                strlen(prefix) == strlen(mount_points[i]->prefix)) {
                return i;
            }
        }
    }

    panic("Mount point not found\n");
    return -1;
}

struct mount_point *vfs_get_mount_point(const int index)
{
    return mount_points[index];
}

int fs_find_free_mount_point()
{
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i] == nullptr) {
            return i;
        }
    }

    return -1;
}

void vfs_add_mount_point(const char *prefix, const uint32_t disk_number, struct inode *inode)
{
    const int index = fs_find_free_mount_point();
    if (index == -1) {
        panic("No free mount points\n");
        return;
    }

    struct mount_point *mount_point = kzalloc(sizeof(struct mount_point));
    if (!mount_point) {
        warningf("Failed to allocate memory for mount point\n");
        return;
    }

    const struct disk *disk = disk_get(disk_number);

    if (disk) {
        mount_point->fs = disk->fs;
    } else {
        mount_point->fs = nullptr;
    }

    mount_point->disk   = disk_number;
    mount_point->prefix = strdup(prefix);
    mount_point->inode  = inode;
    mount_points[index] = mount_point;
}

static void file_free_descriptor(struct file_descriptor *desc)
{
    file_descriptors[desc->index - 1] = nullptr;
    kfree(desc);
}

static int file_new_descriptor(struct file_descriptor **desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == nullptr) {
            struct file_descriptor *desc = kzalloc(sizeof(struct file_descriptor));
            if (desc == nullptr) {
                warningf("Failed to allocate memory for file descriptor\n");
                res = -ENOMEM;
                break;
            }

            // Descriptors start at 1
            desc->index         = i + 1;
            file_descriptors[i] = desc;
            *desc_out           = desc;
            res                 = 0;
            break;
        }
    }

    return res;
}

static struct file_descriptor *file_get_descriptor(const int index)
{
    if (index < 1 || index > MAX_FILE_DESCRIPTORS) {
        return nullptr;
    }

    return file_descriptors[index - 1];
}

struct file_system *vfs_resolve(struct disk *disk)
{
    for (int i = 0; i < MAX_FILE_SYSTEMS; i++) {
        if (file_systems[i] != nullptr) {
            ASSERT(file_systems[i]->resolve != nullptr, "File system does not have resolve function");
            if (file_systems[i]->resolve(disk) == 0) {
                return file_systems[i];
            }
        }
    }

    return nullptr;
}

FILE_MODE file_get_mode(const char *mode)
{
    if (strncmp(mode, "r", 1) == 0) {
        return FILE_MODE_READ;
    }

    if (strncmp(mode, "w", 1) == 0) {
        return FILE_MODE_WRITE;
    }

    if (strncmp(mode, "a", 1) == 0) {
        return FILE_MODE_APPEND;
    }

    return FILE_MODE_INVALID;
}

int vfs_open(const char path[static 1], const char mode[static 1])
{
    int res                     = 0;
    struct disk *disk           = nullptr;
    struct path_root *root_path = path_parser_parse(path, nullptr);
    if (!root_path) {
        warningf("Failed to parse path\n");
        res = -EBADPATH;
        goto out;
    }

    if (!root_path->first) {
        warningf("Path does not contain a file\n");
        res = -EBADPATH;
        goto out;
    }

    if (root_path->drive_number >= 0) {
        disk = disk_get(root_path->drive_number);
        if (!disk) {
            warningf("Failed to get disk\n");
            res = -EIO;
            goto out;
        }

        if (disk->fs == nullptr) {
            warningf("Disk has no file system\n");
            res = -EIO;
            goto out;
        }
    }

    const FILE_MODE file_mode = file_get_mode(mode);
    if (file_mode == FILE_MODE_INVALID) {
        warningf("Invalid file mode\n");
        res = -EINVARG;
        goto out;
    }

    struct inode *inode = nullptr;
    res                 = root_inode_lookup(root_path->first->name, &inode);
    if (ISERR(res)) {
        warningf("Failed to lookup inode\n");
        goto out;
    }

    // ! This means we can only have memfs directories mounted at the root,
    // ! and the device files cannot be in a sub sub directory.
    if (inode->fs_type == FS_TYPE_RAMFS && inode->type == INODE_DIRECTORY) {
        memfs_lookup(inode, root_path->first->next->name, &inode);
    }
    if (inode == nullptr) {
        warningf("Failed to lookup inode\n");
        res = -EBADPATH;
        goto out;
    }

    struct file_descriptor *desc = nullptr;

    void *descriptor_private_data = inode->ops->open(root_path, file_mode);
    if (ISERR(descriptor_private_data)) {
        warningf("Failed to open file\n");
        res = ERROR_I(descriptor_private_data);
        goto out;
    }
    res = file_new_descriptor(&desc);
    if (ISERR(res)) {
        warningf("Failed to create file descriptor\n");
        goto out;
    }
    desc->fs_data = descriptor_private_data;
    if (disk) {
        desc->disk    = disk;
        desc->fs      = disk->fs;
        desc->fs_type = disk->fs->type;
    }

    desc->inode = inode;
    res         = desc->index;

out:
    // fopen returns 0 on error
    // TODO: Implement errno
    if (ISERR(res)) {
        res = 0;
    }

    if (root_path) {
        path_parser_free(root_path);
    }
    return res;
}

int vfs_stat(const int fd, struct file_stat *stat)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->stat(desc, stat);
}

int vfs_seek(const int fd, const int offset, const FILE_SEEK_MODE whence)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->seek(desc, offset, whence);
}

int vfs_read(void *ptr, const uint32_t size, const uint32_t nmemb, const int fd)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->read(desc, size, nmemb, (char *)ptr);
}

int vfs_close(const int fd)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    const int res = desc->inode->ops->close(desc);

    if (res == ALL_OK) {
        file_free_descriptor(desc);
    }

    return res;
}

int vfs_write(const int fd, const char *buffer, const size_t size)
{
    struct file_descriptor *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->write(desc, buffer, size);
}

int vfs_get_non_root_mount_point_count()
{
    int count = 0;
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i] != nullptr &&
            // Skip root mount point
            !(strlen(mount_points[i]->prefix) == 1 && strncmp(mount_points[i]->prefix, "/", 1) == 0)) {
            count++;
        }
    }

    return count;
}

static int memfs_load_directory(struct inode *dir, const struct path_root *root_path)
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

    return res;
}

int vfs_open_dir(const char path[static 1], struct dir_entries **directory)
{
    int res                     = ALL_OK;
    struct path_root *root_path = path_parser_parse(path, nullptr);

    if (root_path == nullptr) {
        warningf("Failed to parse path\n");
        return -EBADPATH;
    }

    struct dir_entries *root_dir = root_inode_get_root_directory();

    // We want to load the root directory
    if (root_path->first == nullptr) {
        *directory = root_dir;
        path_parser_free(root_path);
        return ALL_OK;
    }

    // We want to load a sub directory
    const struct path_part *part = root_path->first;
    struct inode *dir            = nullptr;
    root_inode_lookup(part->name, &dir);

    while (part->next != nullptr) {
        struct inode *next_dir;
        // Try to find it in memory first
        ASSERT(dir->ops->lookup);
        res = dir->ops->lookup(dir, part->next->name, &next_dir);
        if (res < 0) {
            // If it's not in memory, try to load it
            res = memfs_load_directory(dir, root_path);
        }
        part = part->next;
        dir  = next_dir;
    }

    if (dir == nullptr) {
        return -EIO;
    }

    // If the directory has not been initialized, load it
    if (dir->dir_magic != DIR_MAGIC) {
        res = memfs_load_directory(dir, root_path);
    }

    *directory = (struct dir_entries *)dir->data;
    path_parser_free(root_path);

    return res;
}