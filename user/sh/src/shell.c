#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "os.h"
#include "string.h"
#include "types.h"

bool directory_exists(const char *path);
void shell_terminal_readline(char *out, int max, bool output_while_typing);

char *command_history[256];
uint8_t history_index = 0;

int main(int argc, char **argv)
{
    printf(KWHT "User mode shell started\n");

    for (int i = 0; i < 256; i++)
    {
        command_history[i] = malloc(1024);
    }

    while (1)
    {
        char current_directory[MAX_PATH_LENGTH];
        char *current_dir = get_current_directory();
        strncpy(current_directory, current_dir, MAX_PATH_LENGTH);
        printf(KGRN "%s> " KWHT, current_directory);

        char buffer[1024];
        shell_terminal_readline(buffer, sizeof(buffer), true);

        strncpy(command_history[history_index], buffer, sizeof(buffer));
        history_index++;

        if (istrncmp(buffer, "cls", 3) == 0 || istrncmp(buffer, "clear", 5) == 0)
        {
            clear_screen();
            continue;
        }

        if (istrncmp(buffer, "dir", 3) == 0)
        {
            os_system_run("ls", current_directory);
            continue;
        }

        if (strncmp(buffer, "cd", 2) == 0)
        {
            char *new_dir = trim(buffer + 3);
            if (strlen(new_dir) == 0)
            {
                printf("\n");
                continue;
            }

            if (strncmp(new_dir, "0:/", 3) == 0)
            {
                if (!directory_exists(new_dir))
                {
                    printf("\nDirectory does not exist\n");
                    continue;
                }

                if (str_ends_with(new_dir, "/"))
                {
                    set_current_directory(buffer + 3);
                }
                else
                {
                    char *new_path = strcat(new_dir, "/");
                    set_current_directory(new_path);
                }
            }
            else if (strncmp(new_dir, "/", 1) == 0)
            {
                if (!directory_exists(new_dir))
                {
                    printf("\nDirectory does not exist\n");
                    continue;
                }
                char root[MAX_PATH_LENGTH] = "0:";

                set_current_directory(strcat(root, new_dir));
            }
            else if (strncmp(new_dir, "..", 2) == 0)
            {
                int len = strlen(current_directory);
                if (len == 3)
                {
                    printf("\n");
                    continue;
                }
                int i = len - 2;

                while (current_directory[i] != '/')
                {
                    i--;
                }

                current_directory[i] = '/';
                current_directory[i + 1] = '\0';

                if (i == 2)
                {
                    current_directory[3] = '\0';
                }

                set_current_directory(current_directory);
            }
            else
            {
                if (!directory_exists(buffer + 3))
                {
                    printf("\nDirectory does not exist\n");
                    continue;
                }

                if (str_ends_with(new_dir, "/"))
                {
                    char *new_path = strcat(current_directory, new_dir);
                    set_current_directory(new_path);
                }
                else
                {
                    char *new_path = strcat(current_directory, new_dir);
                    set_current_directory(strcat(new_path, "/"));
                }
            }

            printf("\n");
            continue;
        }

        int res = os_system_run(buffer, current_directory);
        if (res < 0)
        {
            printf("\nError: %d", res);
        }

        putchar('\n');
    }

    return 0;
}

bool directory_exists(const char *path)
{
    struct file_directory *directory = malloc(sizeof(struct file_directory));
    char current_directory[MAX_PATH_LENGTH];
    char *current_dir = get_current_directory();
    strncpy(current_directory, current_dir, MAX_PATH_LENGTH);
    int res = 0;

    if (strncmp(path, "0:/", 3) == 0)
    {
        res = opendir(directory, path);
    }
    else if (strncmp(path, "/", 1) == 0)
    {
        char root[MAX_PATH_LENGTH] = "0:";
        char *new_path = strcat(root, path);
        res = opendir(directory, new_path);
    }
    else
    {
        char *new_path = strcat(current_directory, path);
        res = opendir(directory, new_path);
    }

    // opendir(directory, strcat(current_directory, path));
    if (res < 0)
    {
        free(directory);
        return false;
    }

    free(directory);
    return true;
}

void shell_terminal_readline(char *out, int max, bool output_while_typing)
{
    uint8_t current_history_index = history_index;
    int i = 0;
    for (; i < max - 1; i++)
    {
        int key = os_getkey_blocking();

        // Up arrow
        if (key == -30)
        {
            if (current_history_index == 0)
            {
                continue;
            }

            for (int j = 0; j < i - 1; j++)
            {
                os_putchar('\b');
            }
            current_history_index--;
            strncpy(out, command_history[current_history_index], max);
            i = strlen(out);
            printf(out);
            continue;
        }

        // TODO: This is still a little buggy
        // Down arrow
        if (key == -29)
        {
            if (current_history_index == history_index - 1)
            {
                continue;
            }

            for (int j = 0; j < i - 1; j++)
            {
                os_putchar('\b');
            }
            current_history_index++;
            strncpy(out, command_history[current_history_index], max);
            i = strlen(out);
            printf(out);
            continue;
        }

        if (key == '\n' || key == '\r')
        {
            break;
        }

        if (key == '\b' && i <= 0)
        {
            i = -1;
            continue;
        }

        if (output_while_typing)
        {
            os_putchar(key);
        }

        if (key == '\b' && i > 0)
        {
            i -= 2;
            continue;
        }

        out[i] = key;
    }

    out[i] = 0x00;
}