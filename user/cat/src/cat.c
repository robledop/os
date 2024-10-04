#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char **argv)
{
    int fd = fopen("0:/hello.txt", "r");
    printf("File descriptor: %d\n", fd);

    char buffer[1024];

    int res = fread(buffer, 100, 14, fd);
    printf("Read: %d\n", res);
    printf("File contents: %s\n", buffer);

    res = fclose(fd);
    printf("File closed: %d\n", res);

    return 0;
}