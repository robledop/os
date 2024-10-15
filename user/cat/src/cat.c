#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char **argv)
{
    char current_directory[MAX_PATH_LENGTH];
    char *current_dir = get_current_directory();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);

    if (argc < 2)
    {
        printf("\nUsage: cat <file>");
        return -1;
    }

    char file[MAX_PATH_LENGTH];
    strncpy(file, argv[1], MAX_PATH_LENGTH);

    int fd = 0;

    if (starts_with("0:/", file))
    {
        fd = fopen(file, "r");
    }
    else
    {
        char full_path[MAX_PATH_LENGTH];
        strncpy(full_path, current_directory, MAX_PATH_LENGTH);
        strcat(full_path, file);
        fd = fopen(full_path, "r");
    }

    if (fd <= 0)
    {
        printf("\nFailed to open file");
        return fd;
    }

    struct file_stat stat;
    int res = fstat(fd, &stat);
    if (res < 0)
    {
        printf("\nFailed to get file stat");
        return res;
    }

    char* buffer = malloc(stat.size + 1);
    res = fread((void *)buffer, stat.size, 1, fd);
    if (res < 0)
    {
        printf("\nFailed to read file");
        return res;
    }
    buffer[stat.size] = 0x00;

    printf(KCYN "\n%s", buffer);

    fclose(fd);

    return 0;
}