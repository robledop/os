#pragma once

#ifdef __KERNEL__
#error "This is a user-space header file. It should not be included in the kernel."
#endif

#include <config.h>
#include <dirent.h>
#include <printf.h>
#include <stdint.h>
#include <sys/stat.h>
#include <termcolors.h>

typedef struct file_directory_entry (*DIRECTORY_GET_ENTRY)(void *entries, int index);

typedef struct {
    int fd;                 // File descriptor from open()
    char *buffer;           // Pointer to I/O buffer
    size_t buffer_size;     // Size of the buffer
    size_t pos;             // Current position in the buffer
    size_t bytes_available; // Bytes available in the buffer
    int eof;                // End-of-file indicator
    int error;              // Error indicator
    int mode;               // Mode flags (read, write, append, etc.)
} FILE;

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

#define STDIN 0
#define STDOUT 1
#define STDERR 2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int print(const char str[static 1], uint32_t size);
int open(const char name[static 1], int mode);
int close(int fd);
__attribute__((nonnull)) int read(void *ptr, unsigned int size, unsigned int nmemb, int fd);
int write(int fd, const char *buffer, size_t size);
__attribute__((nonnull)) int stat(int fd, struct stat *stat);
int lseek(int fd, int offset, int whence);
void clear_screen();
int mkdir(const char *path);
DIR *opendir(const char *path);
int getkey(void);
int getkey_blocking(void);
char *getcwd(void);
int chdir(const char path[static 1]);
void exit(void);

FILE *fopen(const char *pathname, const char *mode);
int fflush(FILE *stream);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
bool isascii(int c);
