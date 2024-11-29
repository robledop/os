#include <config.h>
#include <os.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, char **argv)
{
    char current_directory[MAX_PATH_LENGTH];
    const char *current_dir = getcwd();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);

    if (argc != 2) {
        printf("\nUsage: cat <file>");
        return -EINVARG;
    }

    char full_path[MAX_PATH_LENGTH];
    char file[MAX_PATH_LENGTH];
    strncpy(file, argv[1], MAX_PATH_LENGTH);

    int fd = 0;

    if (starts_with("/", file)) {
        fd = open(file, O_RDONLY);
    } else {
        strncpy(full_path, current_directory, MAX_PATH_LENGTH);
        strcat(full_path, file);
        fd = open(full_path, O_RDONLY);
    }

    if (fd <= 0) {
        printf("\nFailed to open file: %s", full_path);
        printf("\nError: %s (%d)", get_error_message(fd), fd);
        return fd;
    }

    struct stat s;
    int res = stat(fd, &s);
    if (res < 0) {
        printf("\nFailed to get file stat. File: %s", full_path);
        printf("\nError: %s", get_error_message(res));
        return res;
    }

    char *buffer = malloc(s.st_size + 1);
    res          = read((void *)buffer, s.st_size, 1, fd);
    if (res < 0) {
        printf("\nFailed to read file: %s", full_path);
        printf("\nError: %s", get_error_message(res));
        return res;
    }
    buffer[s.st_size] = 0x00;

    printf(KCYN "\n%s", buffer);


    close(fd);

    return 0;
}