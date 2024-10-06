#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }

    int fd = fopen("0:/", "r");
    printf("\nFile descriptor: %d", fd);

    struct file_stat stat;
    int res = fstat(fd, &stat);

    printf("\nFile size: %d", stat.size);
    printf("\nFlags: %d", stat.flags);

    res = fclose(fd);
    printf("\nFile closed: %d", res);

    return 0;
}