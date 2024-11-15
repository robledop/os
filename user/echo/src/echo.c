#include <stdio.h>
#include <stdlib.h>

int main(const int argc, char **argv)
{
    putchar('\n');
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }

    exit();

    return 0;
}