#include "string.h"

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

char *hex_to_string(uint32_t num)
{
    static char str[11];
    int i = 0;
    str[i++] = '0';
    str[i++] = 'x';
    int j = 268435456;
    while (j)
    {
        int digit = num / j;
        if (digit > 9)
        {
            str[i++] = digit - 10 + 'A';
        }
        else
        {
            str[i++] = digit + '0';
        }
        num %= j;
        j /= 16;
    }
    str[i] = '\0';
    return str;
}

char *int_to_string(int num)
{
    static char str[11];
    int i = 0;
    if (num == 0)
    {
        str[i++] = '0';
    }
    else
    {
        if (num < 0)
        {
            str[i++] = '-';
            num = -num;
        }
        int j = 1000000000;
        while (j > num)
        {
            j /= 10;
        }
        while (j)
        {
            str[i++] = num / j + '0';
            num %= j;
            j /= 10;
        }
    }
    str[i] = '\0';
    return str;
}