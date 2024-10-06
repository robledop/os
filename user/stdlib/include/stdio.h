#ifndef STDIO_H
#define STDIO_H

#define KNRM "\x1B[0m"
#define KBLK "\x1B[30m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

// White foreground on cyan background
#define KWCYN "\x1B[37;46m"

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
void clear_screen();

#endif