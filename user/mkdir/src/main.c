#include <config.h>
#include <status.h>
#include <stdio.h>
#include <string.h>

int main(const int argc, char **argv)
{
    char current_directory[MAX_PATH_LENGTH];
    const char *current_dir = get_current_directory();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);

    if (argc < 2) {
        printf("\nUsage: mkdir <path>");
        return -EINVARG;
    }

    char full_path[MAX_PATH_LENGTH];
    char file[MAX_PATH_LENGTH];
    strncpy(file, argv[1], MAX_PATH_LENGTH);

    int res = 0;

    if (starts_with("/", file)) {
        res = mkdir(file);
    } else {
        strncpy(full_path, current_directory, MAX_PATH_LENGTH);
        strcat(full_path, file);
        res = mkdir(full_path);
    }

    if (res < 0) {
        printf("\nFailed to create folder: %s", full_path);
        printf("\nError: %s (%d)", get_error_message(res), res);
        return res;
    }

    return 0;
}