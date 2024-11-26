#include <kernel.h>
#include <memfs.h>
#include <memory.h>
#include <null.h>
#include <root_inode.h>
#include <status.h>

extern struct inode_operations memfs_directory_inode_ops;

static void *null_open(const struct path_root *path_root, FILE_MODE mode)
{
    return nullptr;
}

static int null_read(const void *descriptor, size_t size, off_t offset, char *out)
{
    return 0;
}

static int null_write(const void *descriptor, const char *buffer, size_t size)
{
    return size;
}

static int null_stat(void *descriptor, struct file_stat *stat)
{
    return 0;
}

static int null_close(void *descriptor)
{
    return 0;
}

static int null_ioctl(void *descriptor, int request, void *arg)
{
    return 0;
}

static int null_seek(void *descriptor, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    return 0;
}

struct inode_operations null_device_fops = {
    .open  = null_open,
    .read  = null_read,
    .write = null_write,
    .stat  = null_stat,
    .seek  = null_seek,
    .close = null_close,
    .ioctl = null_ioctl,
};

int null_init(void)
{
    struct inode *dev_dir = nullptr;

    int res = root_inode_lookup("dev", &dev_dir);
    if (res != 0) {
        root_inode_mkdir("dev", &memfs_directory_inode_ops);
        res = root_inode_lookup("dev", &dev_dir);
        if (ISERR(res) || !dev_dir) {
            return res;
        }
        dev_dir->fs_type = FS_TYPE_RAMFS;
        vfs_add_mount_point("/dev", -1, dev_dir);
    }

    struct inode *null_device = {};
    if (dev_dir->ops->lookup(dev_dir, "null", &null_device) == ALL_OK) {
        return ALL_OK;
    }

    return dev_dir->ops->create_device(dev_dir, "null", &null_device_fops);
}
