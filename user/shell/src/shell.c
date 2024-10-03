#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "os.h"

int main(int argc, char **argv)
{
    printf("Shell started\n");
    while (1)
    {
        printf("> ");
        char buffer[1024];
        os_terminal_readline(buffer, sizeof(buffer), true);
        os_process_start(buffer);
        putchar('\n');
    }
    return 0;
}