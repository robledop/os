#include "stdio.h"
#include <stdarg.h>
#include "../../../src/include/syscall.h"
#include "memory.h"
#include "os.h"
#include "string.h"
#include "syscall.h"

#define MAX_FMT_STR 10240

// FAT Directory entry attributes
#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_HIDDEN 0x02
#define FAT_FILE_SYSTEM 0x04
#define FAT_FILE_VOLUME_LABEL 0x08
#define FAT_FILE_SUBDIRECTORY 0x10
#define FAT_FILE_ARCHIVE 0x20
#define FAT_FILE_DEVICE 0x40
#define FAT_FILE_RESERVED 0x80
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
    syscall0(SYSCALL_CLEAR_SCREEN);
}

void putchar_color(const char c, const unsigned char forecolor, const unsigned char backcolor)
{
    os_putchar_color(c, forecolor, backcolor);
}

int fstat(int fd, struct file_stat *stat)
{
    return syscall2(SYSCALL_STAT, fd, stat);
}

int fopen(const char *name, const char *mode)
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

void putchar(const unsigned char c)
{
    syscall1(SYSCALL_PUTCHAR, c);
}

int print(const char *str)
{
    return syscall1(SYSCALL_PRINT, str);
}

int printf(const char *fmt, ...)
{
    static unsigned char forecolor = 0x0F; // Default white
    static unsigned char backcolor = 0x00; // Default black

    va_list args;

    int x_offset = 0;
    int num      = 0;

    va_start(args, fmt);

    while (*fmt != '\0') {
        char str[MAX_FMT_STR];
        switch (*fmt) {
        case '%':
            memset(str, 0, MAX_FMT_STR);
            switch (*(fmt + 1)) {
            case 'i':
            case 'd':
                num = va_arg(args, int);
                strncpy(str, itoa(num), MAX_FMT_STR);
                // str = itoa(num);
                print(str);
                x_offset += strlen(str);
                break;

            case 'p':
            case 'x':
                num = va_arg(args, int);
                itohex(num, str);
                print("0x");
                print(str);
                x_offset += strlen(str);
                break;

            case 's':
                const char *str_arg = va_arg(args, char *);
                print(str_arg);
                x_offset += strlen(str_arg);
                break;

            case 'c':
                const char char_arg = (char)va_arg(args, int);
                putchar_color(char_arg, forecolor, backcolor);
                x_offset++;
                break;

            default:
                break;
            }
            fmt++;
            break;
        case '\033':
            // Handle ANSI escape sequences
            fmt++;
            if (*fmt != '[') {
                break;
            }
            fmt++;
            int param1 = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                param1 = param1 * 10 + (*fmt - '0');
                fmt++;
            }
            if (*fmt == ';') {
                fmt++;
                int param2 = 0;
                while (*fmt >= '0' && *fmt <= '9') {
                    param2 = param2 * 10 + (*fmt - '0');
                    fmt++;
                }
                if (*fmt == 'm') {
                    switch (param1) {
                    case 30:
                        forecolor = 0x00;
                        break; // Black
                    case 31:
                        forecolor = 0x04;
                        break; // Red
                    case 32:
                        forecolor = 0x02;
                        break; // Green
                    case 33:
                        forecolor = 0x0E;
                        break; // Yellow
                    case 34:
                        forecolor = 0x01;
                        break; // Blue
                    case 35:
                        forecolor = 0x05;
                        break; // Magenta
                    case 36:
                        forecolor = 0x03;
                        break; // Cyan
                    case 37:
                        forecolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    switch (param2) {
                    case 40:
                        backcolor = 0x00;
                        break; // Black
                    case 41:
                        backcolor = 0x04;
                        break; // Red
                    case 42:
                        backcolor = 0x02;
                        break; // Green
                    case 43:
                        backcolor = 0x0E;
                        break; // Yellow
                    case 44:
                        backcolor = 0x01;
                        break; // Blue
                    case 45:
                        backcolor = 0x05;
                        break; // Magenta
                    case 46:
                        backcolor = 0x03;
                        break; // Cyan
                    case 47:
                        backcolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    // Apply the colors to the next characters
                    putchar_color(' ', forecolor, backcolor);
                }
            } else if (*fmt == 'm') {
                switch (param1) {
                case 30:
                    forecolor = 0x00;
                    break; // Black
                case 31:
                    forecolor = 0x04;
                    break; // Red
                case 32:
                    forecolor = 0x02;
                    break; // Green
                case 33:
                    forecolor = 0x0E;
                    break; // Yellow
                case 34:
                    forecolor = 0x01;
                    break; // Blue
                case 35:
                    forecolor = 0x05;
                    break; // Magenta
                case 36:
                    forecolor = 0x03;
                    break; // Cyan
                case 37:
                    forecolor = 0x0F;
                    break; // White
                default:
                    break;
                }
            }
            break;

        default:
            putchar_color(*fmt, forecolor, backcolor);
        }
        fmt++;
    }

    va_end(args);

    return 0;
}

/// @brief Opens a directory for reading
/// @param directory the directory to open
/// @param path the path to the directory to open
/// @return 0 on success
/// @code
/// struct file_directory *directory = malloc(sizeof(struct file_directory));
/// int res = opendir(directory, "pah/to/directory");
int opendir(struct file_directory *directory, const char *path)
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
int readdir(const struct file_directory *directory, struct directory_entry *entry_out, const int index)
{
    const struct fat_directory_entry *entry = directory->entries + (index * sizeof(struct fat_directory_entry));
    struct directory_entry directory_entry  = {
         .attributes           = entry->attributes,
         .size                 = entry->size,
         .access_date          = entry->access_date,
         .creation_date        = entry->creation_date,
         .creation_time        = entry->creation_time,
         .creation_time_tenths = entry->creation_time_tenths,
         .modification_date    = entry->modification_date,
         .modification_time    = entry->modification_time,
         .is_archive           = entry->attributes & FAT_FILE_ARCHIVE,
         .is_device            = entry->attributes & FAT_FILE_DEVICE,
         .is_directory         = entry->attributes & FAT_FILE_SUBDIRECTORY,
         .is_hidden            = entry->attributes & FAT_FILE_HIDDEN,
         .is_long_name         = entry->attributes == FAT_FILE_LONG_NAME,
         .is_read_only         = entry->attributes & FAT_FILE_READ_ONLY,
         .is_system            = entry->attributes & FAT_FILE_SYSTEM,
         .is_volume_label      = entry->attributes & FAT_FILE_VOLUME_LABEL,
    };

    // TODO: Check for a memory leak here
    char *name = trim(substring((char *)entry->name, 0, 7));
    char *ext  = trim(substring((char *)entry->ext, 0, 2));

    directory_entry.name = name;
    directory_entry.ext  = ext;

    *entry_out = directory_entry;

    return 0;
}

// Get the current directory for the current process
char *get_current_directory()
{
    return (char *)syscall0(SYSCALL_GET_CURRENT_DIRECTORY);
}
// Set the current directory for the current process
int set_current_directory(const char *path)
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

    return key;
}
