#include <config.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, char **argv)
{
    char current_directory[MAX_PATH_LENGTH];
    char *current_dir = getcwd();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);
    free(current_dir);

    if (argc != 2) {
        printf("\nUsage: mkdir <path>");
        return -EINVARG;
    }

    char full_path[MAX_PATH_LENGTH];
    char dir[MAX_PATH_LENGTH];
    strncpy(dir, argv[1], MAX_PATH_LENGTH);

    int res = 0;

    if (starts_with("/", dir)) {
        res = mkdir(dir);
    } else {
        strncpy(full_path, current_directory, MAX_PATH_LENGTH);
        strcat(full_path, dir);
        res = mkdir(full_path);
    }

    if (res < 0) {
        printf("\nFailed to create folder: %s", full_path);
        printf("\nError: %s (%d)", get_error_message(res), res);
        return res;
    }

    return 0;
}