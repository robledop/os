#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include "string.h"

int main(int argc, char **argv)
{
    printf("Shell started\n");
    while (1)
    {
        printf("> ");
        char buffer[1024];
        os_terminal_readline(buffer, sizeof(buffer), true);

        if (strncmp(buffer, "cls", 3) == 0 || strncmp(buffer, "clear", 5) == 0)
        {
            clear_screen();
            continue;
        }

        int res = os_system_run(buffer);
        if (res < 0)
        {
            printf("\nError: %d", res);
        }
        putchar('\n');
    }

    return 0;
}