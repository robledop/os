#include "string.h"
#include "kheap.h"

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
void strncpy(char *dest, const char *src, int n)
{
    int i = 0;
    for (i = 0; i < n - 1; i++)
    {
        if (src[i] == '\0')
        {
            break;
        }
        dest[i] = src[i];
    }

    dest[i] = '\0';
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

inline int itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do
    {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        s[i++] = '-';

    s[i] = '\0';
    reverse(s);

    return i;
}

inline int itohex(uint32_t n, char s[])
{
    uint32_t i, d;

    i = 0;
    do
    {
        d = n % 16;
        if (d < 10)
            s[i++] = d + '0';
        else
            s[i++] = d - 10 + 'a';
    } while ((n /= 16) > 0);
    s[i] = 0;
    reverse(s);

    return i;
}

/*
 * Functions from Kerninghan/Ritchie - The C Programming Language
 */

inline void reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
