#include <dirent.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

// FAT Directory entry attributes
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVE 0x20
#define FAT_FILE_LONG_NAME 0x0F

struct fat_directory_entry {
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t first_cluster;
    uint32_t size;
} __attribute__((packed));

void clear_screen()
{
    printf("\033[2J\033[H");
}

int stat(int fd, struct file_stat *stat)
{
    return syscall2(SYSCALL_STAT, fd, stat);
}

int open(const char name[static 1], const int mode)
{
    return syscall2(SYSCALL_OPEN, name, mode);
}

int close(int fd)
{
    return syscall1(SYSCALL_CLOSE, fd);
}

int read(void *ptr, unsigned int size, unsigned int nmemb, int fd)
{
    return syscall4(SYSCALL_READ, ptr, size, nmemb, fd);
}

int write(int fd, const char *buffer, size_t size)
{
    return syscall3(SYSCALL_WRITE, fd, buffer, size);
}

void putchar(char c)
{
    // syscall1(SYSCALL_PUTCHAR, c);
    write(1, &c, 1);
}

int mkdir(const char *path)
{
    return syscall1(SYSCALL_MKDIR, path);
}

// /// @brief Opens a directory for reading
// /// @param directory the directory to open
// /// @param path the path to the directory to open
// /// @return 0 on success
// /// @code
// /// struct file_directory *directory = malloc(sizeof(struct file_directory));
// /// int res = opendir(directory, "pah/to/directory");
// /// \endcode
// int opendir(struct dir_entries *directory, const char path[static 1])
// {
//     return syscall2(SYSCALL_OPEN_DIR, directory, path);
// }

DIR *opendir(const char *name)
{
    const int fd = open(name, O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        return nullptr;
    }

    DIR *dirp = malloc(sizeof(DIR));
    if (dirp == nullptr) {
        close(fd);
        return nullptr;
    }

    dirp->fd     = fd;
    dirp->offset = 0;
    dirp->buffer = nullptr;

    return dirp;
}

#define BUFFER_SIZE 256
struct dirent *readdir(DIR *dirp)
{
    static struct dirent entry;
    static int pos = 0;

    memset(&entry, 0, sizeof(struct dirent));
    if (dirp->buffer == nullptr) {
        dirp->buffer = malloc(BUFFER_SIZE);
        if (dirp->buffer == nullptr) {
            return nullptr;
        }
    }

    if (pos >= dirp->nread) {
        // Refill the buffer
        dirp->nread = getdents(dirp->fd, dirp->buffer, BUFFER_SIZE);
        if (dirp->nread <= 0) {
            pos = 0;
            return nullptr;
        }
        pos = 0;
    }

    const struct dirent *d = (struct dirent *)((void *)dirp->buffer + pos);

    entry.inode_number  = d->inode_number;
    entry.record_length = d->record_length;
    entry.offset        = d->offset;
    strncpy(entry.name, d->name, NAME_MAX);
    entry.name[NAME_MAX - 1] = '\0'; // Ensure null-termination

    pos += d->record_length; // Move to the next entry

    return &entry;
}

int getdents(unsigned int fd, struct dirent *buffer, unsigned int count)
{
    return syscall3(SYSCALL_GETDENTS, fd, buffer, count);
}

// /// @brief Reads an entry from a directory
// /// @param directory the directory to read from
// /// @param entry_out the entry to read into
// /// @param index the index of the entry to read
// /// @return 0 on success
// /// @code
// /// struct file_directory *directory = malloc(sizeof(struct file_directory));
// /// int res = opendir(directory, "pah/to/directory");
// /// for (size_t i = 0; i < directory->entry_count; i++) {
// ///     struct directory_entry entry;
// ///     readdir(directory, &entry, i);
// ///     printf("\n%s", entry.name);
// /// }
// /// free(directory);
// /// \endcode
// int readdir(const struct dir_entries *directory, struct dir_entry **entry_out, const int index)
// {
//     struct dir_entry *entry = directory->entries[index];
//     *entry_out              = entry;
//
//     return 0;
// }

// Get the current directory for the current process
char *get_current_directory()
{
    return (char *)syscall0(SYSCALL_GET_CURRENT_DIRECTORY);
}
// Set the current directory for the current process
int set_current_directory(const char path[static 1])
{
    return syscall1(SYSCALL_SET_CURRENT_DIRECTORY, path);
}
void exit()
{
    syscall0(SYSCALL_EXIT);
}
int getkey()
{
    return syscall0(SYSCALL_GETKEY);
}

int getkey_blocking()
{
    int key = 0;
    key     = getkey();
    while (key == 0) {
        key = getkey();
    }

    __sync_synchronize();

    return key;
}
