#include "os.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

void page_fault_demo();

int main(int argc, char **argv)
{
    putchar('\n');
    for (int i = 1; i < argc; i++)
    {
        printf("%s ", argv[i]);
    }

    return 0;
}