#pragma once

#ifdef __KERNEL__
#error "This is a user-space header file. It should not be included in the kernel."
#endif

#include <config.h>
#include <stdint.h>
#include <termcolors.h>
#include <inode.h>


typedef struct file_directory_entry (*DIRECTORY_GET_ENTRY)(void *entries, int index);

// struct file_directory_entry {
//     char *name;
//     char *ext;
//     uint8_t creation_time_tenths;
//     uint16_t creation_time;
//     uint16_t creation_date;
//     uint16_t access_date;
//     uint16_t modification_time;
//     uint16_t modification_date;
//     uint32_t size;
//     bool is_directory    : 1;
//     bool is_read_only    : 1;
//     bool is_hidden       : 1;
//     bool is_system       : 1;
//     bool is_volume_label : 1;
//     bool is_long_name    : 1;
//     bool is_archive      : 1;
//     bool is_device       : 1;
// };

struct file_directory {
    char *name;
    int entry_count;
    void *entries;
};

struct command_argument {
    char argument[512];
    struct command_argument *next;
    char current_directory[MAX_PATH_LENGTH];
};

struct process_arguments {
    int argc;
    char **argv;
};

typedef unsigned int FILE_SEEK_MODE;
enum { SEEK_SET, SEEK_CURRENT, SEEK_END };

typedef unsigned int FILE_MODE;
enum { FILE_MODE_READ, FILE_MODE_WRITE, FILE_MODE_APPEND, FILE_MODE_INVALID };

typedef unsigned int FILE_STAT_FLAGS;
enum { FILE_STAT_IS_READ_ONLY = 0b00000001 };

struct file_stat {
    FILE_STAT_FLAGS flags;
    unsigned int size;
};

void putchar(unsigned char c);
int printf(const char fmt[static 1], ...);
int print(const char str[static 1], uint32_t size);
int fopen(const char name[static 1], const char mode[static 1]);
int fclose(int fd);

__attribute__((nonnull)) int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd);
__attribute__((nonnull)) int fstat(int fd, struct file_stat *stat);
void clear_screen();
__attribute__((nonnull)) int opendir(struct dir_entries *directory, const char path[static 1]);
__attribute__((nonnull)) int readdir(const struct dir_entries *directory, struct dir_entry **entry_out,
                                     int index);
int getkey(void);
int getkey_blocking(void);

char *get_current_directory(void);
int set_current_directory(const char path[static 1]);

void exit(void);
