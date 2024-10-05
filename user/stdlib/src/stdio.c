#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include <stdarg.h>
#include "string.h"
#include "string.h"

// WARNING: The return value must be freed by the caller
char *normalize_path(const char *path)
{
    if (starts_with("0:/", path))
    {
        char *new_path = (char *)malloc(MAX_PATH_LENGTH);
        strncpy(new_path, path, MAX_PATH_LENGTH);
        return new_path;
    }
    else if (starts_with("/", path))
    {
        char *new_path = (char *)malloc(MAX_PATH_LENGTH);
        if (new_path == NULL)
        {
            return NULL;
        }
        strncpy(new_path, "0:", MAX_PATH_LENGTH);
        strncpy(new_path + 2, path, MAX_PATH_LENGTH - 2);

        return new_path;
    }

    char *new_path = (char *)malloc(MAX_PATH_LENGTH);
    if (new_path == NULL)
    {
        return NULL;
    }
    strncpy(new_path, "0:/", MAX_PATH_LENGTH);
    strncpy(new_path + 3, path, MAX_PATH_LENGTH - 3);

    return new_path;
}

int fstat(int fd, struct file_stat *stat)
{
    return os_stat(fd, stat);
}

int fopen(const char *name, const char *mode)
{
    char *path = normalize_path(name);
    int res = os_open(path, mode);
    free(path);
    return res;
}

int fclose(int fd)
{
    return os_close(fd);
}

int fread(void *ptr, unsigned int size, unsigned int nmemb, int fd)
{
    return os_read(ptr, size, nmemb, fd);
}

int putchar(int c)
{
    os_putchar((char)c);
    return 0;
}

int printf(const char *format, ...)
{
    va_list args;
    const char *p;
    char *sval;
    int ival;

    va_start(args, format);
    for (p = format; *p; p++)
    {
        if (*p != '%')
        {
            putchar(*p);
            continue;
        }

        switch (*++p)
        {
        case 'i':
        case 'd':
            ival = va_arg(args, int);
            os_print(itoa(ival));
            break;
        case 's':
            sval = va_arg(args, char *);
            os_print(sval);
            break;
        default:
            putchar(*p);
            break;
        }
    }

    va_end(args);

    return 0;
}