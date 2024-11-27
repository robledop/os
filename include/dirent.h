#pragma once

#include <stdint.h>
#include "posix.h"

#define NAME_MAX 256

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_CREAT 0x0040
#define O_DIRECTORY 0x10000

struct dirent {
    unsigned long inode_number;
    off_t offset;
    unsigned short record_length;
    unsigned short name_length;
    char name[NAME_MAX];
};

typedef struct {
    int fd;                // File descriptor for the directory
    struct dirent *buffer; // Buffer to hold directory entries
    struct dirent *current_entry;
    off_t offset; // Current position in the directory stream
    int nread;
} DIR;


DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dirp);
int getdents(unsigned int fd, struct dirent *buffer, unsigned int count);
unsigned short dirent_record_length(unsigned short name_length);
