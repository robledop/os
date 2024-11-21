#include <inode.h>
#include <stdio.h>
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

int fstat(int fd, struct file_stat *stat)
{
    return syscall2(SYSCALL_STAT, fd, stat);
}

int fopen(const char name[static 1], const char mode[static 1])
{
    return syscall2(SYSCALL_OPEN, name, mode);
}

int fclose(int fd)
{
    return syscall1(SYSCALL_CLOSE, fd);
}

int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd)
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

/// @brief Opens a directory for reading
/// @param directory the directory to open
/// @param path the path to the directory to open
/// @return 0 on success
/// @code
/// struct file_directory *directory = malloc(sizeof(struct file_directory));
/// int res = opendir(directory, "pah/to/directory");
/// \endcode
int opendir(struct dir_entries *directory, const char path[static 1])
{
    return syscall2(SYSCALL_OPEN_DIR, directory, path);
}

/// @brief Reads an entry from a directory
/// @param directory the directory to read from
/// @param entry_out the entry to read into
/// @param index the index of the entry to read
/// @return 0 on success
/// @code
/// struct file_directory *directory = malloc(sizeof(struct file_directory));
/// int res = opendir(directory, "pah/to/directory");
/// for (size_t i = 0; i < directory->entry_count; i++) {
///     struct directory_entry entry;
///     readdir(directory, &entry, i);
///     printf("\n%s", entry.name);
/// }
/// free(directory);
/// \endcode
int readdir(const struct dir_entries *directory, struct dir_entry **entry_out, const int index)
{
    struct dir_entry *entry = directory->entries[index];
    *entry_out              = entry;

    return 0;
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
