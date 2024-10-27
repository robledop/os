#pragma once

#include "types.h"
#include "../../../src/include/termcolors.h"

// #define KNRM "\x1B[0m"
// #define KRED "\x1B[31m"
// #define KGRN "\x1B[32m"
// #define KYEL "\x1B[33m"
// #define KBLU "\x1B[34m"
// #define KMAG "\x1B[35m"
// #define KCYN "\x1B[36m"
// #define KWHT "\x1B[37m"
// #define KGRY "\x1B[30m"

typedef struct directory_entry (*DIRECTORY_GET_ENTRY)(void *entries, int index);

struct directory_entry {
    char *name;
    char *ext;
    uint8_t attributes;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t modification_time;
    uint16_t modification_date;
    uint32_t size;
    bool is_directory;
    bool is_read_only;
    bool is_hidden;
    bool is_system;
    bool is_volume_label;
    bool is_long_name;
    bool is_archive;
    bool is_device;
};

struct file_directory {
    char *name;
    int entry_count;
    void *entries;
    DIRECTORY_GET_ENTRY get_entry;
};

struct command_argument {
    char argument[512];
    struct command_argument *next;
    char *current_directory;
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
int printf(const char *fmt, ...);
int print(const char *str);
int fopen(const char *name, const char *mode);
int fclose(int fd);
int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd);
int fstat(int fd, struct file_stat *stat);
void clear_screen();
int opendir(struct file_directory *directory, const char *path);
int readdir(const struct file_directory *directory, struct directory_entry *entry, const int index);
int getkey();
int getkey_blocking();

char *get_current_directory();
int set_current_directory(const char *path);

void exit();
