#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(const int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: sleep <time>\n");
        return 1;
    }
    const uint32_t sleep_time = atoi(argv[1]);
    printf("\n");
    sleep(sleep_time);

    return 0;
}