#include "os.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char **argv)
{
    printf("This is from cblank.elf\n");
    printf("Test: %i %s %d\n", 1234, "Hello", 5678);

    void *ptr = malloc(512);

    free(ptr);

    char buffer[512];
    os_terminal_readline(buffer, sizeof(buffer), true);
    printf("\nYou typed: %s\n", buffer);

    printf("Hello, World!\n");

    return 0;
}