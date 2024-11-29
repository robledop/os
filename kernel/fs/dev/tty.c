#include <inode.h>
#include <kernel.h>
#include <memory.h>
#include <root_inode.h>
#include <status.h>
#include <string.h>
#include <tty.h>
#include <vfs.h>
#include <vga_buffer.h>

extern struct inode_operations memfs_directory_inode_ops;

static void *tty_open(const struct path_root *path_root, FILE_MODE mode, enum INODE_TYPE *type_out, uint32_t *size_out)
{
    return nullptr;
}

static int tty_read(const void *descriptor, size_t size, off_t offset, char *out)
{
    memcpy(out, "Not implemented\n", strlen("Not implemented\n"));
    return 0;

    // size_t bytes_read = 0;
    // while (bytes_read < count) {
    //     if (tty_input_buffer_is_empty(tty)) {
    //         // Blocking read: wait for data
    //         wait_for_input(tty);
    //     }
    //     char c = tty_input_buffer_get(tty);
    //     ((char *)buf)[bytes_read++] = c;
    //     if (tty->mode == CANONICAL && c == '\n') {
    //         break; // End of line in canonical mode
    //     }
    // }
    // return bytes_read;
}

static int tty_write(const void *descriptor, const char *buffer, const size_t size)
{
    for (size_t i = 0; i < size; i++) {
        putchar(buffer[i]);
    }

    return (int)size;
}

static int tty_stat(void *descriptor, struct stat *stat)
{
    stat->st_size = strlen("Not implemented\n");
    stat->st_mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    return 0;
}

static int tty_close(void *descriptor)
{
    return 0;
}

static int tty_ioctl(void *descriptor, int request, void *arg)
{
    return 0;
}

static int tty_seek(void *descriptor, uint32_t offset, enum FILE_SEEK_MODE seek_mode)
{
    return 0;
}

struct inode_operations tty_device_fops = {
    .open  = tty_open,
    .read  = tty_read,
    .write = tty_write,
    .stat  = tty_stat,
    .seek  = tty_seek,
    .close = tty_close,
    .ioctl = tty_ioctl,
};

int tty_init(void)
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

    struct inode *tty_device = {};
    if (dev_dir->ops->lookup(dev_dir, "random", &tty_device) == ALL_OK) {
        return ALL_OK;
    }

    return dev_dir->ops->create_device(dev_dir, "tty", &tty_device_fops);
}
