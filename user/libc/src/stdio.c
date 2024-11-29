#include <assert.h>
#include <dirent.h>
#include <memory.h>
#include <status.h>
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

int stat(int fd, struct stat *stat)
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

DIR *opendir(const char *path)
{
    const int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        return nullptr;
    }

    struct stat fstat;
    const int res = stat(fd, &fstat);
    if (res < 0) {
        close(fd);
        return nullptr;
    }

    if (!S_ISDIR(fstat.st_mode)) {
        close(fd);
        return nullptr;
    }

    DIR *dirp = malloc(sizeof(DIR));
    ASSERT(dirp);
    if (dirp == nullptr) {
        close(fd);
        return nullptr;
    }

    ASSERT(dirp != nullptr);

    dirp->fd     = fd;
    dirp->offset = 0;
    dirp->size   = fstat.st_size;
    dirp->buffer = nullptr;

    return dirp;
}

#define BUFFER_SIZE 10240
struct dirent *readdir(DIR *dirp)
{
    ASSERT(dirp != nullptr);

    if ((uint32_t)dirp->offset >= dirp->size) {
        dirp->offset = 0;
        free(dirp->buffer);
        return nullptr;
    }

    static struct dirent entry;
    static uint16_t pos = 0;

    memset(&entry, 0, sizeof(struct dirent));
    if (dirp->buffer == nullptr) {
        dirp->buffer = calloc(BUFFER_SIZE, sizeof(char));
        if (dirp->buffer == nullptr) {
            return nullptr;
        }
    }

    ASSERT(dirp->buffer);

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
    if (d->record_length == 0) {
        return nullptr;
    }

    entry.inode_number  = d->inode_number;
    entry.record_length = d->record_length;
    entry.offset        = d->offset;
    strncpy(entry.name, d->name, NAME_MAX);
    entry.name[NAME_MAX - 1] = '\0'; // Ensure null-termination

    pos += d->record_length; // Move to the next entry
    dirp->offset++;


    return &entry;
}


int closedir(DIR *dir)
{
    if (dir == nullptr) {
        return -1;
    }

    if (dir->buffer) {
        free(dir->buffer);
    }
    close(dir->fd);
    free(dir);

    return ALL_OK;
}

int getdents(unsigned int fd, struct dirent *buffer, unsigned int count)
{
    return syscall3(SYSCALL_GETDENTS, fd, buffer, count);
}

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
