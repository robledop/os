#ifndef STDIO_H
#define STDIO_H

int putchar(int c);
int printf(const char *format, ...);
int fopen(const char *name, const char *mode);
int fclose(int fd);
int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd);

#endif