#include <assert.h>
#include <dirent.h>
#include <memory.h>
#include <status.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

extern int errno;

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

int lseek(int fd, int offset, int whence)
{
    return syscall3(SYSCALL_LSEEK, fd, offset, whence);
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
char *getcwd()
{
    return (char *)syscall0(SYSCALL_GETCWD);
}
// Set the current directory for the current process
int chdir(const char path[static 1])
{
    return syscall1(SYSCALL_CHDIR, path);
}
void exit()
{
    syscall0(SYSCALL_EXIT);
}
int getkey()
{
    int c = 0;
    read(&c, 1, 1, 0); // Read from stdin
    return c;
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

int parse_mode(const char *mode_str, int *flags)
{
    if (strncmp(mode_str, "r", 1) == 0) {
        *flags = O_RDONLY;
    } else if (strncmp(mode_str, "w", 1) == 0) {
        *flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strncmp(mode_str, "a", 1) == 0) {
        *flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strncmp(mode_str, "r+", 2) == 0) {
        *flags = O_RDWR;
    } else if (strncmp(mode_str, "w+", 2) == 0) {
        *flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strncmp(mode_str, "a+", 2) == 0) {
        *flags = O_RDWR | O_CREAT | O_APPEND;
    } else {
        return -1; // Invalid mode
    }
    return 0;
}

#define BUFSIZ 1024

FILE *fopen(const char *pathname, const char *mode)
{
    int flags;
    if (parse_mode(mode, &flags) != 0) {
        // Set errno to EINVAL for invalid argument
        errno = EINVARG;
        return nullptr;
    }

    int fd = open(pathname, flags);
    if (fd == -1) {
        return nullptr;
    }

    FILE *stream = malloc(sizeof(FILE));
    if (!stream) {
        errno = ENOMEM;
        close(fd);
        return nullptr;
    }

    stream->fd          = fd;
    stream->buffer_size = BUFSIZ;
    stream->buffer      = malloc(stream->buffer_size);
    if (!stream->buffer) {
        errno = ENOMEM;
        close(fd);
        free(stream);
        return nullptr;
    }
    stream->pos             = 0;
    stream->bytes_available = 0;
    stream->eof             = 0;
    stream->error           = 0;
    stream->mode            = flags;

    return stream;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t total_bytes = size * nmemb;
    size_t bytes_read  = 0;
    char *dest         = ptr;

    while (bytes_read < total_bytes) {
        if (stream->bytes_available == 0) {
            // Buffer is empty; read from file
            ssize_t n = read(stream->buffer, stream->buffer_size, 1, stream->fd);
            if (n == -1) {
                stream->error = 1;
                return bytes_read / size;
            } else if (n == 0) {
                stream->eof = 1;
                break; // EOF reached
            }
            stream->bytes_available = n;
            stream->pos             = 0;
        }

        size_t bytes_to_copy = stream->bytes_available;
        if (bytes_to_copy > total_bytes - bytes_read) {
            bytes_to_copy = total_bytes - bytes_read;
        }

        memcpy(dest + bytes_read, stream->buffer + stream->pos, bytes_to_copy);
        stream->pos += bytes_to_copy;
        stream->bytes_available -= bytes_to_copy;
        bytes_read += bytes_to_copy;
    }

    return bytes_read / size;
}


int fclose(FILE *stream)
{
    if (!stream) {
        errno = EINVARG;
        return EOF;
    }

    // Flush write buffer if needed
    if (stream->mode & O_WRONLY || stream->mode & O_RDWR) {
        fflush(stream);
    }

    close(stream->fd);

    free(stream->buffer);
    free(stream);

    return 0;
}

int fflush(FILE *stream)
{
    if (!stream) {
        errno = EINVARG;
        return EOF;
    }

    if (stream->mode & O_WRONLY || stream->mode & O_RDWR) {
        // Flush write buffer
        if (stream->pos > 0) {
            ssize_t n = write(stream->fd, stream->buffer, stream->pos);
            if (n == -1) {
                stream->error = 1;
                return EOF;
            }
            stream->pos = 0;
        }
    }

    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t total_bytes   = size * nmemb;
    size_t bytes_written = 0;
    const char *src      = ptr;

    // Error checking
    if (!stream || !ptr || size == 0 || nmemb == 0) {
        return 0;
    }

    // Ensure the stream is writable
    if (!(stream->mode & O_WRONLY || stream->mode & O_RDWR)) {
        stream->error = 1;
        errno         = EBADF;
        return 0;
    }

    while (bytes_written < total_bytes) {
        size_t space_in_buffer = stream->buffer_size - stream->pos;
        size_t bytes_to_copy   = total_bytes - bytes_written;

        if (bytes_to_copy > space_in_buffer) {
            bytes_to_copy = space_in_buffer;
        }

        // Copy data to the buffer
        memcpy(stream->buffer + stream->pos, src + bytes_written, bytes_to_copy);
        stream->pos += bytes_to_copy;
        bytes_written += bytes_to_copy;

        // If buffer is full, flush it
        if (stream->pos == stream->buffer_size) {
            if (fflush(stream) == EOF) {
                // Error occurred during flush
                return bytes_written / size;
            }
        }
    }

    // Return the number of complete elements written
    return bytes_written / size;
}

int fseek(FILE *stream, long offset, int whence)
{
    // Flush write buffer if needed
    if (stream->mode & O_WRONLY || stream->mode & O_RDWR) {
        fflush(stream);
    }

    // Adjust file position
    off_t result = lseek(stream->fd, offset, whence);
    if (result == (off_t)-1) {
        stream->error = 1;
        return -1;
    }

    // Reset buffer
    stream->pos             = 0;
    stream->bytes_available = 0;
    stream->eof             = 0;

    return ALL_OK;
}

int feof(FILE *stream)
{
    return stream->eof;
}

int ferror(FILE *stream)
{
    return stream->error;
}

void clearerr(FILE *stream)
{
    stream->eof   = 0;
    stream->error = 0;
}

bool isascii(int c)
{
    return c >= 0 && c <= 127;
}