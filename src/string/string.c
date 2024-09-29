#include "string.h"
#include "memory/heap/kheap.h"

// Gets the length of a string
size_t strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

// Gets the length of a string
size_t strnlen(const char *s, size_t maxlen)
{
    int n;

    for (n = 0; s[n]; n++)
    {
        if (n == maxlen)
        {
            break;
        }
    }
    return n;
}

int strnlen_terminator(const char *s, size_t maxlen, char terminator)
{
    int n;

    for (n = 0; s[n]; n++)
    {
        if (n == maxlen || s[n] == terminator || s[n] == '\0')
        {
            break;
        }
    }
    return n;
}

// Compare two memory blocks
int memcmp(const void *v1, const void *v2, unsigned int n)
{
    const unsigned char *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0)
    {
        if (*s1 != *s2)
        {
            return *s1 - *s2;
        }
        s1++, s2++;
    }

    return 0;
}

// Compare two strings
int strncmp(const char *p, const char *q, unsigned int n)
{
    while (n > 0 && *p && *p == *q)
    {
        n--, p++, q++;
    }
    if (n == 0)
    {
        return 0;
    }
    return (unsigned char)*p - (unsigned char)*q;
}

char tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}

// Compare two strings ignoring case
int istrncmp(const char *s1, const char *s2, int n)
{
    unsigned char u1, u2;
    while (n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if (u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;
        if (u1 == '\0')
            return 0;
    }

    return 0;
}

// Copy string t to s
char *strncpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
    {
        *s++ = 0;
    }
    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrcpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    if (n <= 0)
    {
        return os;
    }
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
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

bool isdigit(char c)
{
    return c >= '0' && c <= '9';
}

int tonumericdigit(char c)
{
    return c - 48;
}

bool isspace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

// Trim leading and trailing whitespaces
char *trim(char *str)
{
    char *end;
    while (isspace(*str))
    {
        str++;
    }
    if (*str == 0)
    {
        return str;
    }
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
    {
        end--;
    }
    end[1] = '\0';
    return str;
}

char *substring(char *str, int start, int end)
{
    char *substr = (char *)kmalloc(sizeof(char) * (end - start + 2));
    for (int i = start; i <= end; i++)
    {
        substr[i - start] = str[i];
    }
    substr[end - start + 1] = '\0';
    return substr;
}