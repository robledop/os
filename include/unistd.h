#pragma once

int open(const char name[static 1], int mode);
int close(int fd);
__attribute__((nonnull)) int read(void *ptr, unsigned int size, unsigned int nmemb, int fd);
int write(int fd, const char *buffer, size_t size);
int lseek(int fd, int offset, int whence);

