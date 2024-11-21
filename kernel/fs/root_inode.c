#include <disk.h>
#include <kernel_heap.h>
#include <memfs.h>
#include <null.h>
#include <rootfs.h>
#include <string.h>
#include <tty.h>
#include <vfs.h>

extern struct inode_operations memfs_directory_inode_ops;
extern struct mount_point *mount_points[MAX_MOUNT_POINTS];

static struct inode *root_inode;

void root_inode_init(void)
{
    root_inode = memfs_create_inode(INODE_DIRECTORY, &memfs_directory_inode_ops);
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i] == nullptr) {
            continue;
        }

        // If the mount point is the root directory, add the entries to the root inode
        if (strlen(mount_points[i]->prefix) == 1 && strncmp(mount_points[i]->prefix, "/", 1) == 0) {
            const struct disk *disk            = disk_get(mount_points[i]->disk);
            struct dir_entries *root_directory = kzalloc(sizeof(struct dir_entries));
            mount_points[i]->fs->get_root_directory(disk, root_directory);

            for (size_t j = 0; j < root_directory->count; j++) {
                memfs_add_entry_to_directory(
                    root_inode, root_directory->entries[j]->inode, root_directory->entries[j]->name);
            }
        } else {
            memfs_add_entry_to_directory(root_inode, mount_points[i]->inode, mount_points[i]->prefix);
        }
    }

    null_init();
    tty_init();
}

struct inode *root_inode_lookup(const char *name)
{
    struct inode *inode = nullptr;
    memfs_lookup(root_inode, name, &inode);
    return inode;
}

int root_inode_mkdir(const char *name, struct inode_operations *ops)
{
    return memfs_mkdir(root_inode, name, ops);
}

struct dir_entries *root_inode_get_root_directory(void)
{
    return root_inode->data;
}
