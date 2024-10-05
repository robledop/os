#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include <stdarg.h>
#include "string.h"
#include "string.h"
#include "memory.h"

#define MAX_FMT_STR 1024

void clear_screen()
{
    os_clear_screen();
}

void putchar_color(char c, unsigned char forecolor, unsigned char backcolor)
{
    os_putchar_color(c, forecolor, backcolor);
}

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

int printf(const char *fmt, ...)
{
    static unsigned char forecolor = 0x0F; // Default white
    static unsigned char backcolor = 0x00; // Default black

    va_list args;

    int x_offset = 0;
    char str[MAX_FMT_STR];
    int num = 0;

    va_start(args, fmt);

    while (*fmt != '\0')
    {
        switch (*fmt)
        {
        case '%':
            memset(str, 0, MAX_FMT_STR);
            switch (*(fmt + 1))
            {
            case 'i':
            case 'd':
                num = va_arg(args, int);
                strncpy(str, itoa(num), MAX_FMT_STR);
                // str = itoa(num);
                os_print(str);
                x_offset += strlen(str);
                break;

            case 'x':
                num = va_arg(args, int);
                itohex(num, str);
                os_print("0x");
                os_print(str);
                x_offset += strlen(str);
                break;

            case 's':
                char *str_arg = va_arg(args, char *);
                os_print(str_arg);
                x_offset += strlen(str_arg);
                break;

            case 'c':
                char char_arg = (char)va_arg(args, int);
                putchar_color(char_arg, forecolor, backcolor);
                x_offset++;
                break;

            default:
                break;
            }
            fmt++;
            break;
        case '\033':
            // Handle ANSI escape sequences
            fmt++;
            if (*fmt != '[')
            {
                break;
            }
            fmt++;
            int param1 = 0;
            while (*fmt >= '0' && *fmt <= '9')
            {
                param1 = param1 * 10 + (*fmt - '0');
                fmt++;
            }
            if (*fmt == ';')
            {
                fmt++;
                int param2 = 0;
                while (*fmt >= '0' && *fmt <= '9')
                {
                    param2 = param2 * 10 + (*fmt - '0');
                    fmt++;
                }
                if (*fmt == 'm')
                {
                    switch (param1)
                    {
                    case 30:
                        forecolor = 0x00;
                        break; // Black
                    case 31:
                        forecolor = 0x04;
                        break; // Red
                    case 32:
                        forecolor = 0x02;
                        break; // Green
                    case 33:
                        forecolor = 0x0E;
                        break; // Yellow
                    case 34:
                        forecolor = 0x01;
                        break; // Blue
                    case 35:
                        forecolor = 0x05;
                        break; // Magenta
                    case 36:
                        forecolor = 0x03;
                        break; // Cyan
                    case 37:
                        forecolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    switch (param2)
                    {
                    case 40:
                        backcolor = 0x00;
                        break; // Black
                    case 41:
                        backcolor = 0x04;
                        break; // Red
                    case 42:
                        backcolor = 0x02;
                        break; // Green
                    case 43:
                        backcolor = 0x0E;
                        break; // Yellow
                    case 44:
                        backcolor = 0x01;
                        break; // Blue
                    case 45:
                        backcolor = 0x05;
                        break; // Magenta
                    case 46:
                        backcolor = 0x03;
                        break; // Cyan
                    case 47:
                        backcolor = 0x0F;
                        break; // White
                    default:
                        break;
                    }
                    // Apply the colors to the next characters
                    putchar_color(' ', forecolor, backcolor);
                }
            }
            else if (*fmt == 'm')
            {
                switch (param1)
                {
                case 30:
                    forecolor = 0x00;
                    break; // Black
                case 31:
                    forecolor = 0x04;
                    break; // Red
                case 32:
                    forecolor = 0x02;
                    break; // Green
                case 33:
                    forecolor = 0x0E;
                    break; // Yellow
                case 34:
                    forecolor = 0x01;
                    break; // Blue
                case 35:
                    forecolor = 0x05;
                    break; // Magenta
                case 36:
                    forecolor = 0x03;
                    break; // Cyan
                case 37:
                    forecolor = 0x0F;
                    break; // White
                default:
                    break;
                }
            }
            break;

        default:
            putchar_color(*fmt, forecolor, backcolor);
        }
        fmt++;
    }

    va_end(args);

    return 0;
}