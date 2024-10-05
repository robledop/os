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

    int fd = fopen("0:/test", "r");
    printf("\nFile descriptor: %d\n", fd);

    struct file_stat stat;
    int res = fstat(fd, &stat);

    printf("File size: %d\n", stat.size);
    printf("Flags: %d\n", stat.flags);

    res = fclose(fd);
    printf("File closed: %d\n", res);

    return 0;
}