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

    int fd = fopen("0:/hello.txt", "r");
    printf("File descriptor: %d\n", fd);

    int res = fclose(fd);
    printf("File closed: %d\n", res);

    return 0;
}