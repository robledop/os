#include <stdlib.h>


#include "stdio.h"

int main(const int argc, char **argv)
{
    putchar('\n');
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }

    return 0;
}