#include <dev_random.h>
#include <kernel.h>
#include <memfs.h>
#include <memory.h>
#include <rand.h>
#include <root_inode.h>
#include <status.h>

extern struct inode_operations memfs_directory_inode_ops;

static void *random_open(const struct path_root *path_root, FILE_MODE mode)
{
    return nullptr;
}

static int random_read(const void *descriptor, size_t size, off_t offset, char *out)
{
    int res = (int)get_random(); // NOLINT(*-msc50-cpp)
    memcpy(out, &res, sizeof(int));
    return 0;
}

static int random_write(const void *descriptor, const char *buffer, size_t size)
{
    return 0;
}

static int random_stat(void *descriptor, struct file_stat *stat)
{
    stat->size = sizeof(int);
    return 0;
}

static int random_close(void *descriptor)
{
    return 0;
}

static int random_ioctl(void *descriptor, int request, void *arg)
{
    return 0;
}

static int random_seek(void *descriptor, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    return 0;
}

struct inode_operations random_device_fops = {
    .open  = random_open,
    .read  = random_read,
    .write = random_write,
    .stat  = random_stat,
    .seek  = random_seek,
    .close = random_close,
    .ioctl = random_ioctl,
};

int random_init(void)
{
    struct inode *dev_dir = nullptr;
    int res               = root_inode_lookup("dev", &dev_dir);
    if (!dev_dir || res != 0) {
        root_inode_mkdir("dev", &memfs_directory_inode_ops);
        res = root_inode_lookup("dev", &dev_dir);
        if (ISERR(res) || !dev_dir) {
            return res;
        }
        dev_dir->fs_type = FS_TYPE_RAMFS;
        vfs_add_mount_point("/dev", -1, dev_dir);
    }

    struct inode *random_device = {};
    if (dev_dir->ops->lookup(dev_dir, "random", &random_device) == ALL_OK) {
        return ALL_OK;
    }

    return dev_dir->ops->create_device(dev_dir, "random", &random_device_fops);
}
