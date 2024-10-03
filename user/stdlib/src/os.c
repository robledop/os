#include "os.h"

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

        if(key == '\b' && i > 0)
        {
            i -= 2;
            continue;
        }

        out[i] = key;
    }

    out[i] = 0x00;
}