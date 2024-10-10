#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include "string.h"
#include "types.h"

int main(int argc, char **argv)
{
    printf(KWHT "Shell started\n");
    while (1)
    {
        printf(KGRN "> " KWHT);
        char buffer[1024];
        os_terminal_readline(buffer, sizeof(buffer), true);

        if (strncmp(buffer, "cls", 3) == 0 || strncmp(buffer, "clear", 5) == 0)
        {
            clear_screen();
            continue;
        }

        if (strncmp(buffer, "ls", 3) == 0 || strncmp(buffer, "dir", 5) == 0)
        {
            struct file_directory directory;
            opendir(&directory, "0:/test");
            printf("Entries in directory: %d\n", directory.entry_count);
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