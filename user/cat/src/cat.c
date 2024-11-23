#include <config.h>
#include <os.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, char **argv)
{
    char current_directory[MAX_PATH_LENGTH];
    const char *current_dir = get_current_directory();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);

    if (argc < 2) {
        printf("\nUsage: cat <file>");
        return -EINVARG;
    }

    char full_path[MAX_PATH_LENGTH];
    char file[MAX_PATH_LENGTH];
    strncpy(file, argv[1], MAX_PATH_LENGTH);

    int fd = 0;

    if (starts_with("/", file)) {
        fd = open(file, "r");
    } else {
        strncpy(full_path, current_directory, MAX_PATH_LENGTH);
        strcat(full_path, file);
        fd = open(full_path, "r");
    }

    if (fd <= 0) {
        printf("\nFailed to open file: %s", full_path);
        printf("\nError: %s (%d)", get_error_message(fd), fd);
        return fd;
    }

    struct file_stat s;
    int res = stat(fd, &s);
    if (res < 0) {
        printf("\nFailed to get file stat. File: %s", full_path);
        printf("\nError: %s", get_error_message(res));
        return res;
    }

    char *buffer = malloc(s.size + 1);
    res          = read((void *)buffer, s.size, 1, fd);
    if (res < 0) {
        printf("\nFailed to read file: %s", full_path);
        printf("\nError: %s", get_error_message(res));
        return res;
    }
    buffer[s.size] = 0x00;

    printf(KCYN "\n%s", buffer);


    close(fd);

    return 0;
}