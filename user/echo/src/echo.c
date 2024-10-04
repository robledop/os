#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

void page_fault_demo();

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }


    return 0;
}

void page_fault_demo()
{
    char *ptr = malloc(20);
    free(ptr);

    ptr[0] = 'A'; // CRASH!
}