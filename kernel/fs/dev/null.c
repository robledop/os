#include <kernel.h>
#include <memfs.h>
#include <null.h>
#include <rootfs.h>

static int null_read(struct dir_entry *file, char *buffer, size_t size, off_t offset)
{
    return 0;
}

static int null_write(struct dir_entry *file, const char *buffer, size_t size, off_t offset)
{
    return size;
}

struct inode_operations null_device_fops = {
    .read  = null_read,
    .write = null_write,
};

int null_init(void)
{
    struct inode *dev_dir = rootfs_lookup("dev");
    if (!dev_dir) {
        rootfs_mkdir("dev");
        dev_dir = rootfs_lookup("dev");
    }

    fs_add_mount_point("/dev", -1, nullptr, dev_dir);

    return dev_dir->ops->create_device(dev_dir, "null", &null_device_fops);
}
