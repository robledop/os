#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char **argv)
{
    int fd = fopen(argv[1], "r");

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

    char buffer[stat.size + 1];

    res = fread((void *)buffer, stat.size, 1, fd);
    if (res < 0)
    {
        printf("\nFailed to read file");
        return res;
    }
    buffer[stat.size] = 0x00;

    printf(KWCYN "\n%s", buffer);

    fclose(fd);

    return 0;
}