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

#include <fat16.h>


extern int errno;
struct mount_point *mount_points[MAX_MOUNT_POINTS];

struct file_system *file_systems[MAX_FILE_SYSTEMS];
struct file *file_descriptors[MAX_FILE_DESCRIPTORS];

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

static void file_free_descriptor(struct file *desc)
{
    file_descriptors[desc->index - 1] = nullptr;
    if (desc->inode) {
        if (desc->inode->data) {
            kfree(desc->inode->data);
        }
        kfree(desc->inode);
    }
    kfree(desc);
}

static int file_new_descriptor(struct file **desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        if (file_descriptors[i] == nullptr) {
            struct file *desc = kzalloc(sizeof(struct file));
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

static struct file *file_get_descriptor(const uint32_t index)
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

int vfs_open(const char path[static 1], const int mode)
{
    int res = 0;

    struct disk *disk           = nullptr;
    struct path_root *root_path = path_parser_parse(path, nullptr);
    if (!root_path) {
        warningf("Failed to parse path\n");
        res = -EBADPATH;
        goto out;
    }

    if (root_path->drive_number >= 0) {
        disk = disk_get(root_path->drive_number);
        if (!disk) {
            panic("Failed to get disk\n");
            res = -EIO;
            goto out;
        }

        if (disk->fs == nullptr) {
            panic("Disk has no file system\n");
            res = -EIO;
            goto out;
        }
    }

    struct inode *inode = nullptr;

    if (root_path->first != nullptr) {
        root_inode_lookup(root_path->first->name, &inode);

        // ! This means we can only have memfs directories mounted at the root,
        // ! and the device files cannot be in a sub sub directory.
        if (inode && inode->fs_type == FS_TYPE_RAMFS && inode->type == INODE_DIRECTORY) {
            memfs_lookup(inode, root_path->first->next->name, &inode);
        }
    }

    enum INODE_TYPE type;
    void *descriptor_private_data;

    // If the inode is still null, then the file is probably on a disk
    if (inode == nullptr) {
        descriptor_private_data = disk->fs->ops->open(root_path, mode, &type);
    } else {
        descriptor_private_data = inode->ops->open(root_path, mode, &type);
    }


    if (ISERR(descriptor_private_data)) {
        warningf("Failed to open file\n");
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file *desc = nullptr;

    res = file_new_descriptor(&desc);
    if (ISERR(res)) {
        warningf("Failed to create file descriptor\n");
        goto out;
    }
    memcpy(desc->path, path, strlen(path));
    if (disk) {
        desc->disk    = disk;
        desc->fs      = disk->fs;
        desc->fs_type = disk->fs->type;
    }

    desc->fs_data = descriptor_private_data;
    desc->type    = type;

    // HACK: Don't forget to do this properly. The vfs must not know about the file system's internal data
    if (descriptor_private_data) {
        struct fat_file_descriptor *fat_desc = descriptor_private_data;
        desc->fs_data                        = kzalloc(sizeof(struct fat_file_descriptor));
        memcpy(desc->fs_data, fat_desc, sizeof(struct fat_file_descriptor));
    }

    if (inode == nullptr && disk) {
        inode = memfs_create_inode(INODE_FILE, disk->fs->ops);
        memcpy(inode->path, path, strlen(path));
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
    struct file *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->stat(desc, stat);
}

int vfs_seek(const int fd, const int offset, const FILE_SEEK_MODE whence)
{
    struct file *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->seek(desc, offset, whence);
}

int vfs_read(void *ptr, const uint32_t size, const uint32_t nmemb, const int fd)
{
    struct file *desc = file_get_descriptor(fd);
    if (!desc) {
        warningf("Invalid file descriptor\n");
        return -EINVARG;
    }

    return desc->inode->ops->read(desc, size, nmemb, (char *)ptr);
}

int vfs_close(const int fd)
{
    struct file *desc = file_get_descriptor(fd);
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
    struct file *desc = file_get_descriptor(fd);
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

struct dir_entry *read_next_directory_entry(struct file *file)
{
    static struct dir_entry entry;

    const struct inode_operations *dir_ops = file->inode->ops;
    if (dir_ops == nullptr || dir_ops->read_entry == nullptr) {
        errno = ENOTSUP;
        return nullptr;
    }

    const int ret = dir_ops->read_entry(file, &entry);
    if (ret == ALL_OK) {
        return &entry;
    } else if (ret > 0) {
        return nullptr; // End of directory
    } else {
        errno = -ret; // Error code
        return nullptr;
    }
}

// struct dir_entry *read_next_directory_entry(struct file_descriptor *file)
// {
//     static struct dir_entry entry;
//
//     if (!file->type != INODE_DIRECTORY) {
//         errno = ENOTDIR;
//         return nullptr;
//     }
//
//     uint8_t *dir_data       = file->inode->data;
//     const uint32_t dir_size = file->inode->size;
//     const uint32_t offset   = file->offset;
//
//     if (offset >= dir_size) {
//         return nullptr; // End of directory
//     }
//
//     // Read the directory entry at the current offset
//     const struct fs_dir_entry *fs_entry = (struct fs_dir_entry *)(dir_data + offset);
//
//     // Validate record length
//     if (fs_entry->record_length == 0) {
//         errno = EIO; // I/O error or corrupt directory
//         return nullptr;
//     }
//
//     // Advance the offset for the next read
//     file->offset += fs_entry->record_length;
//
//     // Skip entries with inode_number == 0 (deleted entries)
//     if (fs_entry->inode_number == 0) {
//         // Recursively read the next entry
//         return read_next_directory_entry(file);
//     }
//
//     // Populate the in-memory directory entry
//     entry.inode->inode_number = fs_entry->inode_number;
//     entry.file_type           = fs_entry->file_type;
//     // Ensure name_length does not exceed NAME_MAX
//     uint8_t name_len = fs_entry->name_length;
//     if (name_len > NAME_MAX) {
//         name_len = NAME_MAX;
//     }
//     memcpy(entry.name, fs_entry->name, name_len);
//     entry.name[name_len] = '\0'; // Null-terminate the filename
//
//     return &entry;
// }

int copy_to_user(void *dest, const void *src, const size_t size)
{
    if (dest == nullptr || src == nullptr) {
        return -EFAULT;
    }

    memcpy(dest, src, size);
    return 0;
}

int vfs_getdents(const uint32_t fd, void *buffer, const int count)
{
    int bytes_read = 0;

    char *kbuf = kzalloc(count * sizeof(struct dirent));
    if (kbuf == nullptr) {
        return -ENOMEM;
    }

    struct file *file = file_get_descriptor(fd);
    if (file == nullptr || file->type != INODE_DIRECTORY) {
        kfree(kbuf);
        return -EBADF;
    }

    while (bytes_read < count) {
        struct dir_entry *dentry = read_next_directory_entry(file);
        if (dentry == nullptr) {
            break; // End of directory
        }

        auto dirent = (struct dirent *)(kbuf + bytes_read);
        if (dentry->inode) {
            dirent->inode_number = dentry->inode->inode_number;
        }
        dirent->offset        = file->offset;
        dirent->record_length = dirent_record_length(dentry->name_length);

        // Copy the filename
        memcpy(dirent->name, dentry->name, dentry->name_length);
        dirent->name[dentry->name_length] = '\0';

        bytes_read += dirent->record_length;
    }

    // Copy the kernel buffer to user space
    if (copy_to_user(buffer, kbuf, bytes_read)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return bytes_read;
}

int vfs_open_dir(const char path[static 1], struct dir_entries **dir_entries)
{
    int res = ALL_OK;

    struct path_root *root_path = path_parser_parse(path, nullptr);

    if (root_path == nullptr) {
        warningf("Failed to parse path\n");
        return -EBADPATH;
    }

    // We want to load the root directory
    if (root_path->first == nullptr) {
        struct inode *root_inode = root_inode_get();

        const struct disk *disk = disk_get(root_path->drive_number);
        path_parser_free(root_path);
        struct dir_entries *root_directory = kzalloc(sizeof(struct dir_entries));

        res = disk->fs->get_root_directory(disk, root_directory);

        for (size_t j = 0; j < root_directory->count; j++) {
            memfs_add_entry_to_directory(
                root_inode, root_directory->entries[j]->inode, root_directory->entries[j]->name);
        }

        *dir_entries = root_inode->data;
        return res;
    }

    // We want to load a sub directory
    const struct path_part *part = root_path->first;
    struct inode *dir            = {};
    root_inode_lookup(part->name, &dir);

    while (part->next != nullptr) {
        struct inode *next_dir = {};
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

    *dir_entries = (struct dir_entries *)dir->data;
    path_parser_free(root_path);

    return res;
}

int vfs_mkdir(const char *path)
{
    const struct path_root *root_path = path_parser_parse(path, nullptr);
    if (root_path == nullptr) {
        warningf("Failed to parse path\n");
        return -EBADPATH;
    }
    struct inode *inode = {};

    const int res = root_inode_lookup(root_path->first->name, &inode);

    if (res < 0) {
        const struct disk *disk = disk_get(root_path->drive_number);
        if (disk == nullptr) {
            warningf("Failed to get disk\n");
            return -EIO;
        }
        return disk->fs->ops->mkdir(path);
    } else {
        return inode->ops->mkdir(path);
    }
}
