#pragma once

#ifdef __KERNEL__
#error "This is a user-space header file. It should not be included in the kernel."
#endif

#include <config.h>
#include <dirent.h>
#include <printf.h>
#include <stat.h>
#include <stdint.h>
#include <termcolors.h>


typedef struct file_directory_entry (*DIRECTORY_GET_ENTRY)(void *entries, int index);

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

int print(const char str[static 1], uint32_t size);
int open(const char name[static 1], int mode);
int close(int fd);
__attribute__((nonnull)) int read(void *ptr, unsigned int size, unsigned int nmemb, int fd);
int write(int fd, const char *buffer, size_t size);
__attribute__((nonnull)) int stat(int fd, struct file_stat *stat);
void clear_screen();
int mkdir(const char *path);
// __attribute__((nonnull)) int opendir(struct dir_entries *directory, const char path[static 1]);
DIR *opendir(const char *name);
// __attribute__((nonnull)) int readdir(const struct dir_entries *directory, struct dir_entry **entry_out, int index);
int getkey(void);
int getkey_blocking(void);
char *get_current_directory(void);
int set_current_directory(const char path[static 1]);
void exit(void);
