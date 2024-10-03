#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

void crash_demo();

int main(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        printf("%s \n", argv);
    }

    return 0;
}

void crash_demo()
{
    char *ptr = malloc(20);
    free(ptr);

    ptr[0] = 'A'; // CRASH!
}