#ifndef STDIO_H
#define STDIO_H

typedef unsigned int FILE_SEEK_MODE;
enum
{
    SEEK_SET,
    SEEK_CURRENT,
    SEEK_END
};

typedef unsigned int FILE_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

typedef unsigned int FILE_STAT_FLAGS;
enum
{
    FILE_STAT_IS_READ_ONLY = 0b00000001
};

struct file_stat
{
    FILE_STAT_FLAGS flags;
    unsigned int size;
};

int putchar(int c);
int printf(const char *format, ...);
int fopen(const char *name, const char *mode);
int fclose(int fd);
int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd);
int fstat(int fd, struct file_stat *stat);

#endif