#include "os.h"
#include "string.h"

struct command_argument *os_parse_command(const char *command, int max)
{
    struct command_argument *head = NULL;
    char scommand[1025];
    if (max >= (int)sizeof(scommand))
    {
        return NULL;
    }

    strncpy(scommand, command, sizeof(scommand));
    char *token = strtok(scommand, " ");

    if (token == NULL)
    {
        return head;
    }

    head = os_malloc(sizeof(struct command_argument));
    if (head == NULL)
    {
        return NULL;
    }

    strncpy(head->argument, token, sizeof(head->argument));
    head->next = NULL;

    struct command_argument *current = head;
    token = strtok(NULL, " ");

    while (token != NULL)
    {
        struct command_argument *next = os_malloc(sizeof(struct command_argument));
        if (next == NULL)
        {
            break;
        }

        strncpy(next->argument, token, sizeof(next->argument));
        next->next = NULL;
        current->next = next;
        current = next;
        token = strtok(NULL, " ");
    }

    return head;
}

int os_getkey_blocking()
{
    int key = 0;
    while ((key = os_getkey()) == 0)
    {
    }

    return key;
}

void os_terminal_readline(char *out, int max, bool output_while_typing)
{
    int i = 0;
    for (; i < max - 1; i++)
    {
        int key = os_getkey_blocking();
        if (key == '\n' || key == '\r')
        {
            break;
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

int os_system_run(const char *command)
{
    char buffer[1024];
    strncpy(buffer, command, sizeof(buffer));
    struct command_argument *root_command_argument = os_parse_command(buffer, sizeof(buffer));
    if (root_command_argument == NULL)
    {
        return -1;
    }

    return os_system(root_command_argument);
}